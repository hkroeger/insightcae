/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "openfoam/caseelements/analysiscaseelements.h"
#include "base/intervals.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include "base/boost_include.h"
#include "base/translations.h"
#include <iterator>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace std
{
bool operator==(const insight::OFDictData::data& d, const std::string& s)
{
    const std::string* s1=boost::get<std::string>(&d);
    if (!s1) return false;
    return (*s1==s);
}
bool operator==(const std::string& s, const insight::OFDictData::data& d)
{
    return d==s;
}
}

namespace insight
{


std::unique_ptr<std::pair<time_t,boost::filesystem::path> >
newestOutputFile(const path &expectedFName)
{
    // find newest out file
    std::map<std::time_t, boost::filesystem::path> candidates;

    auto fileNameBase = expectedFName.filename().stem().string();
    auto fileNameExt = expectedFName.filename().extension().string();
    auto pd = expectedFName.parent_path();

    boost::regex expr(fileNameBase+"(|_.*)"+fileNameExt);

    for ( directory_iterator itr( pd );
         itr != directory_iterator(); ++itr )
    {
        if (!is_directory(itr->status()))
        {
            auto cp = itr->path();
            if (boost::regex_match(cp.filename().string(), expr))
            {
                candidates[last_write_time(cp)]=cp;
            }
        }
    }

    if (candidates.size()<1)
    {
        insight::Warning( _("no valid output file was found in time directory %s!"), pd.c_str());
    }
    else
    {
        // select newest (last in list)
        auto selectedCandidate = candidates.rbegin();
        return std::make_unique<std::pair<time_t,boost::filesystem::path> >(*selectedCandidate);
    }

    return nullptr;
}



class TabularInterval : public Interval
{
    arma::mat table_;
public:
    TabularInterval(const arma::mat& tab)
        : Interval( tab(0,0), tab(tab.n_rows-1, 0) ),
          table_(tab)
    {}

    arma::mat clippedTable() const
    {
        CurrentExceptionContext ex(
                    str(format("clipping table %g < t <%g") % A() % clippedB())
                    );
        insight::assertion( !toBeIgnored(), "no data!" );
        return arma::mat( table_.rows( find(table_.col(0)>A() && table_.col(0)<clippedB()) ) );
    }
};




std::map<std::string, arma::mat>
readSingleTabularFile(
        const boost::filesystem::path& ffp,
        int groupByColumn,
        const std::string& filterChars
        )
{
    std::map<std::string, std::vector<std::vector<double> > > rows;


    std::ifstream f( ffp.string() );
    if (!f)
      throw insight::Exception("Failed to open file "+ffp.string()+"!");

    int lineNo=0;
    std::string line;
    while ( getline ( f, line ) )
    {
      lineNo++;
      CurrentExceptionContext ex(3, str(format("reading line %d of file %s")%lineNo%ffp.string()));

      trim(line);

      if ( !starts_with ( line, "#" ) )
      {
        for (auto c: filterChars)
          erase_all(line, std::string(1, c));
        replace_all(line, "\t", " ");

        // eliminate double spaces
        string line_org;
        do {
          line_org=line;
          replace_all(line, "  ", " ");
        } while (line_org!=line);

        std::vector<string> fields;
        split(fields, line,  boost::is_any_of(" "));

        std::string groupName="default";
        if (groupByColumn>=0)
        {
          groupName=fields[groupByColumn];
          fields.erase(fields.begin()+groupByColumn);
        }

        std::vector<double> fieldValues;
        transform(
              fields.begin(), fields.end(), std::back_inserter(fieldValues),
              [](const std::string& t) { return toNumber<double>(t); }
        );

        if (fieldValues.size()<2)
          throw insight::Exception("invalid data: expected at least two columns (time + 1 data), got: "+line);


        rows[groupName].push_back(fieldValues);
      }
    }

    std::map<std::string, arma::mat> result;
    // convert into arma::mat's
    for (const auto& rg: rows)
    {
        arma::mat m = arma::zeros(rg.second.size(), rg.second.front().size());
        for (long int i=0; i<rg.second.size(); ++i)
        {
            m.row(i)=arma::mat(rg.second[i].data(), 1, rg.second[i].size());
        }
//        insight::dbg()<<rg.first<<":\n"<<m<<std::endl;
        result[rg.first]=m;
    }
    return result;
}



arma::mat readAndCombineTabularFiles
(
    const OpenFOAMCase& cm, const boost::filesystem::path& caseLocation,
    const std::string& FOName, const std::string& fileNamePattern,
    const std::string& filterChars,
    const std::string& regionName
)
{
  return readAndCombineGroupedTabularFiles(
               cm, caseLocation,
               FOName, fileNamePattern,
               -1, filterChars, regionName )
          .begin()->second;
}



std::map<std::string,arma::mat> readAndCombineGroupedTabularFiles
(
    const OpenFOAMCase& cm, const boost::filesystem::path& caseLocation,
    const std::string& FOName, const std::string& fileNamePattern,
    int groupByColumn,
    const std::string& filterChars,
    const std::string& regionName
)
{
  CurrentExceptionContext ex("reading output files "+fileNamePattern+" for function object "+FOName+" (filtering out any of '"+filterChars+"')");


  path fp;
  if ( cm.OFversion() <170 )
  {
    fp = absolute ( caseLocation ) / FOName;
  }
  else
  {
    fp = absolute ( caseLocation ) / "postProcessing";
    if (!regionName.empty()) fp = fp / regionName;
    fp = fp/ FOName;
  }

  if (!exists(fp))
  {
      throw insight::Exception(
        _("data path %s of function object %s does not exist!"),
        fp.c_str(), FOName.c_str() );
  }

  std::map<std::string, OverlappingIntervals> intervals;

  // find all time directories
  auto tdl = listTimeDirectories ( fp );

  std::time_t lastWriteTime=0;
  for ( const auto& td: tdl ) // loop over all times, start
  {
      auto newest = newestOutputFile(td.second/fileNamePattern);
      if (newest)
      {
            auto ffp = newest->second;
      if (lastWriteTime > newest->first)
      {
        insight::Warning(
              _("Possible inconsistency in solver output data detected!"
                "File %s from time directory %s"
                " was created before the output file of the previous time directory."),
               ffp.filename().c_str(), td.second.c_str()
              );
      }
      lastWriteTime=newest->first;

      auto fileData = readSingleTabularFile(ffp, groupByColumn, filterChars);
      for (const auto& rg: fileData)
      {
          intervals[rg.first].insert(
                      lastWriteTime,
                      std::make_shared<TabularInterval>(rg.second) );
      }
    }
  }

  for (auto& iv: intervals)
  {
      iv.second.clipIntervals();
  }

  std::map<std::string, arma::mat> rdata;

  for (const auto& giv: intervals)
  {
      auto& rows=rdata[giv.first];
      for (const auto& iv: giv.second.intervals())
      {
          const auto& tiv = dynamic_cast<const TabularInterval&>(*iv.second);
          if (rows.n_rows==0)
          {
              rows=tiv.clippedTable();
          }
          else
          {
              rows=arma::join_cols(rows, tiv.clippedTable());
          }
      }
  }

//  for (const auto& rg: rows)
//  {
//    const auto& crows=rg.second;

//    arma::mat data;

//    if (crows.size()>0)
//    {
//      size_t nf = crows.begin()->second.size();
//      data.resize(crows.size(), nf);

//      arma::uword k=0;
//      for (auto r=crows.begin(); r!=crows.end(); ++r)
//      {

//        if ( nf != r->second.size() )
//        {
//          throw insight::Exception(
//                str(format("Invalid data for time %g: expected %d data columns, got %d.")
//                    % r->first % nf % r->second.size()
//                    ));
//        }

//        for (arma::uword j=0; j<nf; j++)
//        {
//          data(k,j)=r->second[j];
//        }

//        ++k;
//      }
//    }

//    rdata[rg.first]=data;
//  }

  return rdata;
}





defineType(outputFilterFunctionObject);
defineFactoryTable
(
    outputFilterFunctionObject,
    LIST
    (
        OpenFOAMCase& c,
        ParameterSetInput&& ip
    ),
    LIST ( c, std::move(ip) )
);
defineStaticFunctionTable(outputFilterFunctionObject, defaultParameters, std::unique_ptr<ParameterSet>);




outputFilterFunctionObject::outputFilterFunctionObject(
    OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, /*Parameters(ps).name+"outputFilterFunctionObject",*/
                          ip.forward<Parameters>())
{
}

std::vector<string> outputFilterFunctionObject::requiredLibraries() const
{
    return {};
}




void outputFilterFunctionObject::addIntoControlDict(OFDictData::dict& controlDict) const
{
  OFDictData::dict fod=functionObjectDict();
  fod["region"]=p().region;
  fod["enabled"]=true;
  if (OFversion()>=400)
    {
      fod["writeControl"]=p().outputControl;
      fod["writeInterval"]=p().outputInterval;
    }
  else
    {
      fod["outputControl"]=p().outputControl;
      fod["outputInterval"]=p().outputInterval;
    }
  fod["timeStart"]=p().timeStart;

  controlDict.subDict("functions")[p().name]=fod;

  auto libs=requiredLibraries();
  for (const auto& l: libs)
  {
      controlDict.getList("libs").insertNoDuplicate(l);
  }
}




void outputFilterFunctionObject::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  addIntoControlDict(controlDict);
}




void outputFilterFunctionObject::evaluate
(
  OpenFOAMCase& , const boost::filesystem::path& , ResultSetPtr& ,
  const std::string&
) const
{
}




defineType(fieldAveraging);
addToOpenFOAMCaseElementFactoryTable(fieldAveraging);




fieldAveraging::fieldAveraging(OpenFOAMCase& c,  ParameterSetInput ip)
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}




OFDictData::dict fieldAveraging::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="fieldAverage";
  OFDictData::list libl; libl.push_back("\"libfieldFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  fod["enabled"]=true;
  
  OFDictData::list fl;
  for (const std::string& fln: p().fields)
  {
    fl.push_back(fln);
    OFDictData::dict cod;
    cod["mean"]=true;
    cod["prime2Mean"]=true;
    cod["base"]="time";
    fl.push_back(cod);
  }
  fod["fields"]=fl;
  
  return fod;
}
  
  
  
  
defineType(probes);
addToOpenFOAMCaseElementFactoryTable(probes);




probes::probes(OpenFOAMCase& c,  ParameterSetInput ip )
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}




OFDictData::dict probes::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="probes";
  OFDictData::list libl; libl.push_back("\"libsampling.so\"");
  fod["functionObjectLibs"]=libl;
  
  OFDictData::list pl;
  for (const arma::mat& lo: p().probeLocations)
  {
    pl.push_back(OFDictData::vector3(lo));
  }
  fod["probeLocations"]=pl;
  
  OFDictData::list fl; fl.resize(p().fields.size());
  copy(p().fields.begin(), p().fields.end(), fl.begin());
  fod["fields"]=fl;
  
  return fod;
}


arma::mat readTensor(std::istream& is)
{
  CurrentExceptionContext ex("reading tensor from input stream");

  std::vector<double> data;
  string s;
  do {
    is >> s;
    if (is.fail()) break;
    double d; std::istringstream(s) >> d;
    data.push_back(d);
  } while (!is.eof() && !ends_with(s, ")"));
  
  return arma::mat(data.data(), 1, data.size());
}

arma::cube probes::readProbes 
( 
    const OpenFOAMCase& c,
    const boost::filesystem::path& location,
    const std::string& foName, 
    const std::string& fieldName 
)
{
  CurrentExceptionContext ex("reading probes data of field "+fieldName+" from function object "+foName+" in case directory \""+location.string()+"\"");

  typedef std::vector<arma::mat> Instant;
  typedef std::map<double, Instant> History;
  
  History sample_history;
  int ncmpt=-1, npts=-1;
  
  
  path fp = absolute ( location ) /"postProcessing"/foName;
  
  if ( c.OFversion() < 400 )
      insight::Warning("readProbes has not been tested for the current OF version "+boost::lexical_cast<std::string>(c.OFversion()));

  if (!exists(fp))
      throw insight::Exception("data path of function object "+foName+" does not exist!");
  
  // find all time directories
  TimeDirectoryList tdl=listTimeDirectories ( fp );
  for ( const TimeDirectoryList::value_type& td: tdl )
  {
    boost::filesystem::path ffp = td.second/fieldName;

    if (!exists(ffp))
        insight::Warning("field "+fieldName+" was not found in time directory "+td.second.string()+" of probes function object "+foName+"!");
        
    std::ifstream f( ffp.c_str() );
    while ( !f.eof() )
    {
        std::string line;

        getline ( f, line );
        if ( f.fail() ) break;
        if ( !starts_with ( line, "#" ) )
        {
            std::istringstream os(line);

            double time;
            os >> time;

            Instant cur_data;
            while (!os.eof())
            {
                arma::mat v;
                char c;
                os >> c;
                if (c=='(')
                {
                    v = readTensor(os);
                }
                else
                {
                    os.unget();
                    v=arma::zeros(1);
                    os >> v(0);
                }

                if (ncmpt>0) // check num of compt
                {
                    if (ncmpt!=v.n_elem)
                        throw std::string("incorrect number of components in probes data! (@time %g: found %d, expected %d)");
                }
                else
                {
                    ncmpt=v.n_elem;
                }

                cur_data.push_back(v);
            }

            if (npts>0) // check number of sample points
            {
                if (npts!=cur_data.size())
                    throw std::string("incorrect number of sample points in probes data! (@time %g: found %d, expected %d)");
            }
            else
            {
                npts=cur_data.size();
            }

            sample_history[time]=cur_data;

        }
    }
  }

  int ninstants=sample_history.size();

  arma::cube data = arma::zeros(ninstants, npts+1, ncmpt);
  int i=0;
  for (const History::value_type& instant: sample_history)
  {
    int j=0;
    for (int k=0; k<ncmpt; k++) data(i, 0, k)=instant.first;
        
    for (const Instant::value_type& sample: instant.second)
    {
        for (int k=0; k<ncmpt; k++) data(i, j+1, k)=sample(k);
        j+=1;
    }
    i+=1;
  }
    
  return data;
}



arma::mat probes::readProbesLocations
(
    const OpenFOAMCase&,
    const boost::filesystem::path& location, 
    const std::string& foName
)
{
  CurrentExceptionContext ex("reading location of probe points of function object "+foName+" from controlDict in case \""+location.string()+"\"");

  OFDictData::dict controlDict;
  readOpenFOAMDict(location/"system"/"controlDict", controlDict);
  
  OFDictData::dict& fol = controlDict.subDict("functions");
  
  if (fol.find(foName)==fol.end())
      throw insight::Exception("Function object \""+foName+"\" not found in controlDict!");
  
  OFDictData::dict& fod = fol.subDict(foName);
  OFDictData::list& plo=fod.getList("probeLocations");
  
  arma::mat data = arma::zeros(plo.size(), 3);
  for (arma::uword i=0; i<plo.size(); i++)
  {
      OFDictData::list& row = boost::get<OFDictData::list>(plo[i]);
      for (arma::uword k=0; k<3; k++) data(i,k)=as_scalar(row[k]);
  }
  return data;
}


defineType(volumeIntegrate);
addToOpenFOAMCaseElementFactoryTable(volumeIntegrate);

volumeIntegrate::volumeIntegrate(OpenFOAMCase& c,  ParameterSetInput ip )
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}

OFDictData::dict volumeIntegrate::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="volFieldValue";

  OFDictData::list libl; libl.push_back("\"libfieldFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;

  if ( const Parameters::domain_wholedomain_type* dt =
       boost::get<Parameters::domain_wholedomain_type>(&p().domain) )
    {
      fod["regionType"]="all";
    }
  else if ( const Parameters::domain_cellZone_type* dcz =
       boost::get<Parameters::domain_cellZone_type>(&p().domain) )
    {
      fod["regionType"]="cellZone";
      fod["name"]=dcz->cellZoneName;
    }

  fod["writeFields"]=false;
  string op;
  if (p().operation == Parameters::volIntegrate)
    op="volIntegrate";
  else if (p().operation == Parameters::sum)
    op="sum";
  else if (p().operation == Parameters::sumMag)
    op="sumMag";
  else if (p().operation == Parameters::average)
    op="average";
  else if (p().operation == Parameters::volAverage)
    op="volAverage";
  else if (p().operation == Parameters::min)
    op="min";
  else if (p().operation == Parameters::max)
    op="max";
  else if (p().operation == Parameters::CoV)
    op="CoV";
  else if (p().operation == Parameters::weightedVolIntegrate)
  {
    op="weightedVolIntegrate";
    fod["weightField"]=p().weightFieldName;
  }
  fod["operation"]=op;

  OFDictData::list fl; fl.resize(p().fields.size());
  copy(p().fields.begin(), p().fields.end(), fl.begin());
  fod["fields"]=fl;

  return fod;
}

arma::mat volumeIntegrate::readVolumeIntegrals
(
    const OpenFOAMCase& c,
    const boost::filesystem::path& location,
    const std::string& FOName,
    const std::string& regionName
)
{
  return readAndCombineTabularFiles
      (
        c, location,
        FOName, "volFieldValue.dat",
        "()", regionName
      );
}

defineType(surfaceIntegrate);
addToOpenFOAMCaseElementFactoryTable(surfaceIntegrate);

surfaceIntegrate::surfaceIntegrate(OpenFOAMCase& c,  ParameterSetInput ip)
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}

OFDictData::dict surfaceIntegrate::functionObjectDict() const
{
//  insight::Warning("incomplete implementation");
  OFDictData::dict fod;

  OFDictData::list libl; libl.push_back("\"libfieldFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  OFDictData::list fl; fl.resize(p().fields.size());
  copy(p().fields.begin(), p().fields.end(), fl.begin());
  fod["fields"]=fl;

  if (OFversion()<170)
  {
    // foam-extend
    fod["type"]="faceSource";

    if ( const Parameters::domain_patch_type* dp =
         boost::get<Parameters::domain_patch_type>(&p().domain) )
      {
        fod["source"]="patch";
        fod["sourceName"]=dp->patchName;
      }
    else if ( const Parameters::domain_faceZone_type* dcz =
         boost::get<Parameters::domain_faceZone_type>(&p().domain) )
      {
        fod["source"]="faceZone";
        fod["sourceName"]=dcz->faceZoneName;
      }

    if (p().operation == Parameters::operation_type::areaIntegrate)
      {
        fod["operation"]="areaIntegrate";
      }
    else if (p().operation == Parameters::operation_type::areaAverage)
      {
        fod["operation"]="areaAverage";
      }
    else if (p().operation == Parameters::operation_type::sum)
      {
        fod["operation"]="sum";
      }

    fod["valueOutput"]=false;
    fod["log"]=true;
  }
  else
  {
    fod["type"]="surfaceFieldValue";

    if ( const Parameters::domain_patch_type* dp =
         boost::get<Parameters::domain_patch_type>(&p().domain) )
      {
        fod["regionType"]="patch";
        fod["name"]=dp->patchName;
      }
    else if ( const Parameters::domain_faceZone_type* dcz =
         boost::get<Parameters::domain_faceZone_type>(&p().domain) )
      {
        fod["regionType"]="faceZone";
        fod["name"]=dcz->faceZoneName;
      }

    fod["writeFields"]=false;

    if (p().operation == Parameters::operation_type::areaIntegrate)
      {
        fod["operation"]="areaIntegrate";
      }
    else if (p().operation == Parameters::operation_type::areaAverage)
      {
        fod["operation"]="areaAverage";
      }
    else if (p().operation == Parameters::operation_type::sum)
      {
        fod["operation"]="sum";
      }

  }

  return fod;
}

arma::mat surfaceIntegrate::readSurfaceIntegrate
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    const std::string& foName,
    const std::string& regionName
)
{

  return readAndCombineTabularFiles
      (
          cm, location,
          foName,
          (cm.OFversion()<170 ? "faceSource.dat" : "surfaceFieldValue.dat"),
          "()", regionName
      );
//  arma::mat result(0,2);

//  boost::filesystem::path dir;
//  if (cm.OFversion()<170)
//  {
//    dir = location / foName;
//  }
//  else
//  {
//    dir = location / "postProcessing";
//    if (!regionName.empty())
//      dir = dir / regionName;
//    dir = dir / foName;
//  }
//  auto tdl = listTimeDirectories(dir);
//  for(decltype(tdl)::const_reverse_iterator i = tdl.crbegin(); i!=tdl.crend(); i++)
//  {
//    auto f = i->second /
//        (cm.OFversion()<170 ? "faceSource.dat" : "surfaceFieldValue.dat");

//    auto newest = newestOutputFile(f);

//    std::ifstream fs(f.c_str());
//    arma::mat cr = readTextFile(fs);

//    if (result.n_rows==0)
//    {
//      result=cr;
//    }
//    else
//    {
//      double tmax=result(0,0);
//      result = arma::join_cols
//               (
//                 cr.rows( arma::find( cr.col(0) < tmax ) ),
//                 result
//               );
//    }
//  }

//  if (cm.OFversion()<170)
//  {
//    return arma::join_rows(result.col(0), result.col(2));
//  }
//  else
//  {
//    return result;
//  }
}

defineType(fieldMinMax);
addToOpenFOAMCaseElementFactoryTable(fieldMinMax);

fieldMinMax::fieldMinMax(OpenFOAMCase& c,  ParameterSetInput ip )
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}

OFDictData::dict fieldMinMax::functionObjectDict() const
{
//  insight::Warning("incomplete implementation");

  OFDictData::dict fod;
  fod["type"]="fieldMinMax";

  OFDictData::list libl; libl.push_back("\"libfieldFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;

  fod["writeFields"]=false;


  OFDictData::list fl; fl.resize(p().fields.size());
  copy(p().fields.begin(), p().fields.end(), fl.begin());
  fod["fields"]=fl;

  return fod;
}

std::map<std::string,arma::mat> fieldMinMax::readOutput
(
    const OpenFOAMCase& c,
    const boost::filesystem::path& location,
    const std::string& FOName,
    const std::string& regionName
)
{
  return readAndCombineGroupedTabularFiles
      (
        c, location,
        FOName, "fieldMinMax.dat",
        1, "()", regionName
      );
}


defineType(cuttingPlane);
addToOpenFOAMCaseElementFactoryTable(cuttingPlane);

cuttingPlane::cuttingPlane(OpenFOAMCase& c, ParameterSetInput ip)
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}

OFDictData::dict cuttingPlane::functionObjectDict() const
{
  OFDictData::dict fod;

  fod["type"]="surfaces";
  OFDictData::list l;
  l.assign<string>(list_of<string>("\"libsampling.so\""));
  fod["functionObjectLibs"]=l;
  fod["interpolationScheme"]="cellPoint";

  fod["surfaceFormat"]="vtk";

  // Fields to be sampled
  std::copy(
      p().fields.begin(), p().fields.end(),
      std::back_inserter(l)
      );
  // l.assign<string>(p().fields);
  fod["fields"]=l;

  OFDictData::dict pd;
  pd["type"]="cuttingPlane";
  pd["planeType"]="pointAndNormal";
  pd["interpolate"]=false;
  OFDictData::dict pand;
  pand["basePoint"]="(0 0 0)";
  pand["normalVector"]="(0 0 1)";
  pd["pointAndNormalDict"]=pand;

  OFDictData::list sl;
  sl.push_back(p().name);
  sl.push_back(pd);
  fod["surfaces"]=sl;
  
  return fod;
}

   
   
   
defineType(twoPointCorrelation);
addToOpenFOAMCaseElementFactoryTable(twoPointCorrelation);

twoPointCorrelation::twoPointCorrelation(OpenFOAMCase& c, ParameterSetInput ip)
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{
}

OFDictData::dict twoPointCorrelation::csysConfiguration() const
{
  OFDictData::data 
    e1=OFDictData::vector3(1,0,0), 
    e2=OFDictData::vector3(0,1,0);
    
  OFDictData::dict csys;
  csys["type"]="cartesian";
  csys["origin"]=OFDictData::vector3(0,0,0);
  if (OFversion()>=230)
  {
    OFDictData::dict top, crd;
    crd["type"]="axesRotation";
    crd["e1"]=e1;
    crd["e2"]=e2;
    csys["coordinateRotation"]=crd;
    top["coordinateSystem"]=csys;
    return top;
  } 
  else
  {
    csys["e1"]=e1;
    csys["e2"]=e2;
    return csys;
  }
}

OFDictData::dict twoPointCorrelation::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="twoPointCorrelation";
  OFDictData::list libl; libl.push_back("\"libLESFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  fod["enabled"]=true;
  fod["outputControl"]=p().outputControl;
  fod["outputInterval"]=p().outputInterval;
  fod["timeStart"]=p().timeStart;
  
  fod["p0"]=OFDictData::vector3(p().p0);
  fod["directionSpan"]=OFDictData::vector3(p().directionSpan);
  fod["np"]=p().np;
  fod["homogeneousTranslationUnit"]=OFDictData::vector3(p().homogeneousTranslationUnit);
  fod["nph"]=p().nph;

  fod["csys"]=csysConfiguration();
  
  return fod;
}

boost::ptr_vector<arma::mat> twoPointCorrelation::readCorrelations
(
    const OpenFOAMCase& c,
    const boost::filesystem::path& location,
    const std::string& tpcName
)
{
  CurrentExceptionContext ex("reading correlation data of function object "+tpcName+" in case \""+location.string()+"\"");
  int nk=9;
  
  std::vector<double> t; // time step array
  std::vector<double> profs[nk]; // profiles
  int np=-1;
  
  path fp;
  if (c.OFversion()<170)
    fp=absolute(location)/tpcName;
  else
    fp=absolute(location)/"postProcessing"/tpcName;
  
  TimeDirectoryList tdl=listTimeDirectories(fp);
  
  for (const TimeDirectoryList::value_type& td: tdl)
  {
    std::ifstream f( (td.second/"twoPointCorrelation.dat").c_str());
    while (!f.eof())
    {
      string line;
      getline(f, line);
      if (f.fail()) break;

      if (!starts_with(line, "#"))
      {
	
	// split into component tpcs
	std::vector<string> strs;
	boost::split(strs, line, boost::is_any_of("\t"));
	
        t.push_back(toNumber<double>(strs[0]));
	
	if (strs.size()!=(nk+2))
	  throw insight::Exception("Expected "
				    +lexical_cast<string>(nk+1)
				    +" profiles in twoPointCorrelation results but got "
				    +lexical_cast<string>(strs.size()-1)+"!");
	
	for (int k=1; k<=nk; k++)
	{
	  std::vector<string> pts;
	  boost::split(pts, strs[k], boost::is_any_of(" "));
	  if (np<0) 
	    np=pts.size()-1; // trailing space!
	  else if (np!=pts.size()-1)
	    throw insight::Exception("Expected uniform number of sampling point in twoPointCorrelation results!");
	  
	  for (int j=0; j<np; j++)
          {
            profs[k-1].push_back(toNumber<double>(pts[j]));
	  }
	}
      }
    }
  }
  
  ptr_vector<arma::mat> res;
  res.push_back(new arma::mat(t.data(), t.size(), 1));
  for (int k=0; k<nk; k++) res.push_back(new arma::mat( arma::mat(profs[k].data(), np, t.size()).t() ) );
  
  return res;  
}




defineType(cylindricalTwoPointCorrelation);
addToOpenFOAMCaseElementFactoryTable(cylindricalTwoPointCorrelation);

cylindricalTwoPointCorrelation::cylindricalTwoPointCorrelation(
    OpenFOAMCase& c, ParameterSetInput ip)
: twoPointCorrelation(c, ip.forward<Parameters>())
{}

OFDictData::dict cylindricalTwoPointCorrelation::csysConfiguration() const
{
  OFDictData::dict csys;
  csys["type"]="cylindrical";
  csys["origin"]=OFDictData::vector3(0,0,0);
  csys["degrees"]=p().degrees;
  
  OFDictData::data 
    e1=OFDictData::vector3(p().er),
    e3=OFDictData::vector3(p().ez);
    
  if (OFversion()>=230)
  {
    OFDictData::dict top, crd;
    crd["type"]="axesRotation";
    crd["e1"]=e1;
    crd["e3"]=e3;
    csys["coordinateRotation"]=crd;
    top["coordinateSystem"]=csys;
    return top;
  } 
  else
  {
    csys["e1"]=e1;
    csys["e3"]=e3;
    return csys;
  }
  
}




defineType(forces);
addToOpenFOAMCaseElementFactoryTable(forces);

forces::forces(OpenFOAMCase& c,  ParameterSetInput ip)
: outputFilterFunctionObject(c, ip.forward<Parameters>())
{}

OFDictData::dict forces::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="forces";

  fod["log"]=true;
  
  OFDictData::list pl;
  for (const std::string& lo: p().patches)
  {
    pl.push_back(lo);
  }
  fod["patches"]=pl;
  if (OFversion()>=400)
    {
      fod["p"]=p().pName;
      fod["U"]=p().UName;
      fod["rho"]=p().rhoName;
    }
  else
    {
      fod["pName"]=p().pName;
      fod["UName"]=p().UName;
      fod["rhoName"]=p().rhoName;
    }
  fod["rhoInf"]=p().rhoInf;
  
  fod["CofR"]=OFDictData::vector3(p().CofR);
  
  return fod;
}

std::vector<string> forces::requiredLibraries() const
{
    return { "\"libforces.so\"" };
}


void cleanFileContent(string& file)
{
  erase_all ( file, "(" );
  erase_all ( file, ")" );
  replace_all ( file, ",", " " );
  replace_all ( file, "\t", " " );
  while (file.find("  ")!=std::string::npos)
  {
    replace_all ( file, "  ", " " );
  }
}

void readForcesLine(std::istream& f, int nc_expected, bool& skip, std::vector<double>& row)
{
  CurrentExceptionContext ex("reading a line from forces file", false);

  std::string line;

  getline ( f, line );
  
  if ( f.fail() ) { row.clear(); return; }

  if ( starts_with ( line, "#" ) ) { skip=true; row.clear(); return; };

  std::vector<string> strs;
  boost::split ( strs, line, boost::is_any_of ( " " ) );

  if ( strs.size() != nc_expected )
  {
      row.clear();
  }

  row.resize(strs.size());

  int k=0;
  for ( const string& e: strs )
  {
    row[k++] = toNumber<double> ( e );
  }

}

arma::mat forces::readForces
(
    const OpenFOAMCase& c,
    const boost::filesystem::path& location,
    const std::string& foName
)
{
  CurrentExceptionContext ex("reading output of forces function object "+foName+" in case \""+location.string()+"\"");

  std::vector<std::vector<double> >  fl;

  path fp;
  if ( c.OFversion() <170 )
    fp=absolute ( location ) /foName;
  else
    fp=absolute ( location ) /"postProcessing"/foName;

  TimeDirectoryList tdl=listTimeDirectories ( fp );

  int ncexp=13;
  if ( c.OFversion() >=230 ) ncexp=19;
  if ( c.OFversion() >=300 ) ncexp=10;

  for ( const TimeDirectoryList::value_type& td: tdl )
  {
    path f_name, f2_name;
    stringstream f, f2;

    if ( c.OFversion() >=300 )
    {
      f_name=( td.second/"force.dat" );
      f2_name=( td.second/"moment.dat" );
    }
    else
    {
      f_name=( td.second/"forces.dat" );
    }

    {
      std::ifstream ff;
      ff.open ( f_name.c_str() );
      istreambuf_iterator<char> endf;
      std::string fc( istreambuf_iterator<char>(ff), endf );
      cleanFileContent(fc);
      f.str(fc);
    }

    if (!f2_name.empty())
    {
      std::ifstream ff2;
      ff2.open ( f2_name.c_str() );
      istreambuf_iterator<char> endf;
      std::string fc2( istreambuf_iterator<char>(ff2), endf );
      cleanFileContent(fc2);
      f2.str(fc2);
    }

    int line_num=0;
    while ( !f.eof() )
    {
      line_num++;

//      if (line_num%1000) cout<<"."<<flush;

        bool skip=false;

        std::vector<double> row;
        if ( c.OFversion() >=300 )
          {
            std::vector<double> r1, r2;
            {
              CurrentExceptionContext ex(3, str(format("reading line %d from files \"%s\"")%line_num%f_name.string()), false);
              readForcesLine ( f, ncexp, skip, r1 );
            }
            {
              CurrentExceptionContext ex(3, str(format("reading line %d from files \"%s\"")%line_num%f2_name.string()), false);
              readForcesLine ( f2, ncexp, skip, r2 );
            }

            if ( (r1.size()==0) || (r2.size()==0) )
            {
                if ( !skip ) break;
            }
            else
            {
                // make compatible with earlier OF versions
                r1.erase(r1.begin()+1,r1.begin()+3+1); // remove total force
                r2.erase(r2.begin()+1,r2.begin()+3+1); // remove total moment
                r2.erase(r2.begin()); // remove time column

                row=r1;
                row.insert( row.end(), r2.begin(), r2.end() );
            }
          }
        else
          {
            CurrentExceptionContext ex(3, str(format("reading line %d from file \"%s\"")%line_num%f_name.string()), false);
            readForcesLine ( f, ncexp, skip, row );
            if ( row.size()==0 )
            {
                if ( !skip ) break;
            }
            else
            {
                if ( c.OFversion() >=220 )
                {
                    // remove porous forces
                    row.erase ( row.begin()+16, row.begin()+18+1 );
                    row.erase ( row.begin()+7, row.begin()+9+1 );
                } 
            }
        }

        if (row.size()>0)
        {
          if (fl.size()>0)
          {
            if (fl.back().size()!=row.size())
              throw insight::Exception("Inconsistent number of columns in forces file!");
          }
          fl.push_back(row);
        }
      }
  }

  arma::mat result;
  if (fl.size()>0)
  {
    auto nr=fl.size(), nc=fl.back().size();
    result=arma::zeros(nr, nc);
    for (size_t r=0; r<nr; r++)
      for (size_t c=0; c<nc; c++)
      {
        result(r, c)=fl[r][c];
      }
  }
  return result;
}




defineType(extendedForces);
addToOpenFOAMCaseElementFactoryTable(extendedForces);

extendedForces::extendedForces(OpenFOAMCase& c, ParameterSetInput ip)
: forces(c, ip.forward<Parameters>())
{}

OFDictData::dict extendedForces::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="extendedForces";

  fod["log"]=true;
  
  OFDictData::list pl;
  for (const std::string& lo: p().patches)
  {
    pl.push_back(lo);
  }
  fod["patches"]=pl;
  fod["pName"]=p().pName;
  fod["UName"]=p().UName;
  if (OFversion()>=400)
    fod["rho"]=p().rhoName;
  else
    fod["rhoName"]=p().rhoName;
  fod["rhoInf"]=p().rhoInf;
  
  if (p().maskField!="")
  {
      fod["maskField"]=p().maskField;
      fod["maskThreshold"]=p().maskThreshold;
  }
  
  fod["CofR"]=OFDictData::vector3(p().CofR);
  return fod;
}

std::vector<string> extendedForces::requiredLibraries() const
{
    return { "\"libextendedForcesFunctionObject.so\"" };
}

  


defineType(catalyst);
addToOpenFOAMCaseElementFactoryTable(catalyst);

catalyst::catalyst(OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, /*"catalyst", */ip.forward<Parameters>())
{
}

void catalyst::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict fod;
  fod["type"]="catalyst";
  OFDictData::list libl; libl.push_back("\"libcatalystFoam.so\"");
  fod["functionObjectLibs"]=libl;

  fod["executeControl"]="timeStep";
  fod["writeControl"]="none";

  OFDictData::list scripts;

  for (const auto& ss: p().scripts)
  {
    if (const auto* sc = boost::get<Parameters::scripts_default_copy_type>(&ss))
    {
      scripts.push_back("\"<system>/"+sc->filename->fileName().string()+"\"");
    }
    else if (const auto* sg = boost::get<Parameters::scripts_default_generate_type>(&ss))
    {
      scripts.push_back("\"<system>/"+sg->name+".py\"");
    }
    else throw insight::UnhandledSelection();
  }

  fod["scripts"]=scripts;

  OFDictData::dict& inputs = fod.subDict("inputs");
  OFDictData::dict& inputregion = inputs.subDict(p().inputname);
  inputregion["boundary"]=true;

  OFDictData::list fieldlist;
  std::transform(p().fields.begin(), p().fields.end(), std::back_inserter(fieldlist),
                 [](const Parameters::fields_default_type& f) { return f; }
  );
  inputregion["fields"]=fieldlist;

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.subDict("functions")["catalyst"]=fod;
}


void catalyst::modifyCaseOnDisk ( const OpenFOAMCase&, const boost::filesystem::path& location ) const
{

  for (const auto& ss: p().scripts)
  {

    if (const auto* sc = boost::get<Parameters::scripts_default_copy_type>(&ss))
    {
//      boost::filesystem::copy_file(sc->filename, location/"system");
      sc->filename->copyTo( location/"system"/sc->filename->fileName() );
    }

    else if (const auto* sg = boost::get<Parameters::scripts_default_generate_type>(&ss))
    {
      std::ofstream f( (location/"system"/(sg->name+".py")).c_str() );

      f<<
      "from paraview.simple import *\n"
      "from paraview import coprocessing\n"

      "def CreateCoProcessor():\n"
      "  def _CreatePipeline(coprocessor, datadescription):\n"
      "    class Pipeline:\n"
      "      paraview.simple._DisableFirstRenderCameraReset()\n"
      "      dataset = coprocessor.CreateProducer(datadescription, '"<<p().inputname<<"')\n";

      if (sg->contour)
        f<<
      "      contour = Contour(Input=dataset)\n"
            ;

      if (sg->slice)
        f<<
      "      slice = Slice(Input=dataset)\n"
      "      slice.SliceType = 'Plane'\n"
            ;

      if (sg->extractBlock)
        f<<
      "      extractBlock = ExtractBlock(Input=dataset)\n"
            ;

      f<<
      "    return Pipeline()\n"

      "  class CoProcessor(coprocessing.CoProcessor):\n"
      "    def CreatePipeline(self, datadescription):\n"
      "      self.Pipeline = _CreatePipeline(self, datadescription)\n"

      "  coprocessor = CoProcessor()\n"
      "  freqs = {'"<<p().inputname<<"': []}\n"
      "  coprocessor.SetUpdateFrequencies(freqs)\n"
      "  return coprocessor\n"

      "coprocessor = CreateCoProcessor()\n"
      "coprocessor.EnableLiveVisualization(True, 1)\n"

      "def RequestDataDescription(datadescription):\n"
      "    global coprocessor\n"
      "    if datadescription.GetForceOutput() == True:\n"
      "        for i in range(datadescription.GetNumberOfInputDescriptions()):\n"
      "            datadescription.GetInputDescription(i).AllFieldsOn()\n"
      "            datadescription.GetInputDescription(i).GenerateMeshOn()\n"
      "        return\n"
      "    coprocessor.LoadRequestedData(datadescription)\n"

      "def DoCoProcessing(datadescription):\n"
      "    global coprocessor\n"
      "    coprocessor.UpdateProducers(datadescription)\n"
      "    coprocessor.WriteData(datadescription);\n"
      "    coprocessor.WriteImages(datadescription, rescale_lookuptable=False)\n"
      "    coprocessor.DoLiveVisualization(datadescription, \""<<p().paraview_host<<"\", "<<p().paraview_port<<")\n"
      ;

    }

    else throw insight::UnhandledSelection();

  }
}



CorrelationFunctionModel::CorrelationFunctionModel()
: A_(1.0), B_(1.0), omega_(1.0), phi_(0.0)
{}

int CorrelationFunctionModel::numP() const
{
  return 4;
}

void CorrelationFunctionModel::setParameters(const double* params)
{
  B_=max(0.1, params[0]);
  omega_=params[1];
  phi_=params[2];
  A_=params[3];
}

void CorrelationFunctionModel::setInitialValues(double* params) const
{
  params[0]=0.1;
  params[1]=0.1;
  params[2]=0.0;
  params[3]=1.0;
}

arma::mat CorrelationFunctionModel::weights(const arma::mat& x) const
{
  return exp( -x / max(x.col(0)) );
}

arma::mat CorrelationFunctionModel::evaluateObjective(const arma::mat& x) const
{
  //cout<<B_<<" "<<omega_<<" "<<x.col(0)<<endl;
  return A_ * exp(-B_*x.col(0)) % ( cos(omega_*x.col(0) -phi_) );
}

double CorrelationFunctionModel::lengthScale() const
{
  return A_ * ( B_*cos(phi_) + omega_*sin(phi_) ) / ( pow(B_, 2) + pow(omega_, 2) );
}


// addToAnalysisFactoryTable(ComputeLengthScale);
defineType(ComputeLengthScale);
Analysis::Add<ComputeLengthScale> addComputeLengthScale;

ComputeLengthScale::ComputeLengthScale(
    const std::shared_ptr<supplementedInputDataBase>& sp  )
  : AnalysisWithParameters(sp)
{}

ResultSetPtr ComputeLengthScale::operator()(ProgressDisplayer&)
{
  auto results = createResultSet();

  CorrelationFunctionModel m;
  nonlinearRegression(p().R_vs_x.col(1), p().R_vs_x.col(0), m);

  arma::mat x = arma::linspace(0., max(p().R_vs_x.col(0)), 100);
  arma::mat regressiondata
  (
      arma::join_rows ( x, m.evaluateObjective(x) )
  );

  results->insert( "L", new ScalarResult(m.lengthScale(), "Length scale", "", "m") );
  addPlot
  (
      results, executionPath(), "chartACF",
      "x", "$\\langle R \\rangle$",
      {
        PlotCurve(p().R_vs_x, "raw", "w p lt 1 t 'ACF'"),
        PlotCurve(regressiondata, "fit", "w l lt 2 t 'fit'")
      },
      "two-point correlation function"
  );

  return results;
}


template<>
const char * LinearTPCArray::cmptNames[] = 
{ "xx", "xy", "xz",
  "yx", "yy", "yz",
  "zx", "zy", "zz"
};

template<>
typename twoPointCorrelation::Parameters LinearTPCArray::getTanParameters(int i) const
{ 
    typename twoPointCorrelation::Parameters r;
    r.set_p0( p().p0 + r_.back()*p().e_rad );
    r.set_directionSpan( p().tanSpan*p().e_tan );
    r.set_np(p().np);
    r.set_homogeneousTranslationUnit( (p().tanSpan/double(p().nph))*p().e_tan );
    r.set_nph( p().nph );

    r.set_name(p().name+"_tan_"+lexical_cast<std::string>(i));
    r.set_outputControl("timeStep");
    r.set_timeStart( p().timeStart );
    return r;
}

template<>
typename twoPointCorrelation::Parameters LinearTPCArray::getAxParameters(int i) const
{
    typename twoPointCorrelation::Parameters r;
    r.set_p0( p().p0 + r_.back()*p().e_rad );
    r.set_directionSpan( p().axSpan*p().e_ax );
    r.set_np(p().np);
    r.set_homogeneousTranslationUnit( (p().tanSpan/double(p().nph))*p().e_tan );
    r.set_nph(p().nph);
    r.set_name(p().name+"_ax_"+lexical_cast<std::string>(i));
    r.set_outputControl("timeStep");
    r.set_timeStart( p().timeStart );
    return r;
}

template<>
std::string LinearTPCArray::axisTitleTan() const
{
   return "Tangential distance [length]";
}

template<>
std::string LinearTPCArray::axisTitleAx() const
{
   return "Axial distance [length]";
}

template<>
const char * RadialTPCArray::cmptNames[] = 
{ "xx", "xy", "xz",
  "yx", "yy", "yz",
  "zx", "zy", "zz"
};

template<>
typename cylindricalTwoPointCorrelation::Parameters RadialTPCArray::getTanParameters(int i) const
{
    typename cylindricalTwoPointCorrelation::Parameters r;
    r.set_er(vec3(0, 1, 0));
    r.set_ez(vec3(1, 0, 0));
    r.set_degrees(false);

    r.set_p0( p().p0 + r_.back()*p().e_rad );
    r.set_directionSpan( p().tanSpan*p().e_tan );
    r.set_np(p().np);
    r.set_homogeneousTranslationUnit( (2.*M_PI/double(p().nph))*p().e_tan );
    r.set_nph( p().nph );

    r.set_name(p().name+"_tan_"+lexical_cast<std::string>(i));
    r.set_outputControl("timeStep");
    r.set_timeStart( p().timeStart );
    return r;
}

template<>
typename cylindricalTwoPointCorrelation::Parameters RadialTPCArray::getAxParameters(int i) const
{
    typename cylindricalTwoPointCorrelation::Parameters r;
    r.set_er(vec3(0, 1, 0));
    r.set_ez(vec3(1, 0, 0));
    r.set_degrees(false);

    r.set_p0( p().p0 + r_.back()*p().e_rad );
    r.set_directionSpan( p().axSpan*p().e_ax );
    r.set_np(p().np);
    r.set_homogeneousTranslationUnit( (2.*M_PI/double(p().nph))*p().e_tan );
    r.set_nph(p().nph);

    r.set_name(p().name+"_ax_"+lexical_cast<std::string>(i));
    r.set_outputControl("timeStep");
    r.set_timeStart( p().timeStart );
    return r;
}

template<>
std::string RadialTPCArray::axisTitleTan() const
{
   return "Angle [rad]";
}

template<>
std::string RadialTPCArray::axisTitleAx() const
{
   return "Axial distance [length]";
}

const char LinearTPCArrayTypeName[] = "LinearTPCArray";
// defineType(LinearTPCArray);
template<> const std::string LinearTPCArray::typeName( LinearTPCArray::typeName_() );
addToFactoryTable(OpenFOAMCaseElement, LinearTPCArray);
addToStaticFunctionTable(OpenFOAMCaseElement, LinearTPCArray, defaultParameters);
addToStaticFunctionTable(OpenFOAMCaseElement, LinearTPCArray, category);
addToFactoryTable(outputFilterFunctionObject, LinearTPCArray);
addToStaticFunctionTable(outputFilterFunctionObject, LinearTPCArray, defaultParameters);

const char RadialTPCArrayTypeName[] = "RadialTPCArray";
// defineType(RadialTPCArray);
template<> const std::string RadialTPCArray::typeName( RadialTPCArray::typeName_() );
addToFactoryTable(OpenFOAMCaseElement, RadialTPCArray);
addToStaticFunctionTable(OpenFOAMCaseElement, RadialTPCArray, defaultParameters);
addToStaticFunctionTable(OpenFOAMCaseElement, RadialTPCArray, category);
addToFactoryTable(outputFilterFunctionObject, RadialTPCArray);
addToStaticFunctionTable(outputFilterFunctionObject, RadialTPCArray, defaultParameters);


}
