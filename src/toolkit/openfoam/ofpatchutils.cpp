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

#include "ofpatchutils.h"
#include "openfoam/oftimedirectories.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"

#include "base/units.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "base/warningdispatcher.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


void setSet(
    const OpenFOAMCase& ofc,
    const boost::filesystem::path& location,
    const std::vector<std::string>& cmds )
{
  CurrentExceptionContext ex("executing setSet command with the instructions:\n"+boost::join(cmds, "\n"));

  std::vector<std::string> opts;
  if ((ofc.OFversion()>=220) && (listTimeDirectories(location).size()==0)) opts.push_back("-constant");
  std::string machine=""; // problems, if job is put into queue system

  auto job = ofc.forkCommand(location, "setSet", opts, &machine);

  for (const std::string& line: cmds)
  {
        job->input() << line << endl;
  }
  job->input() << "quit" << endl;
  job->closeInput();

  std::vector<std::string> errout;
  job->runAndTransferOutput(nullptr, &errout);

  int retcode=job->process().exit_code();
  if (retcode!=0)
  {
    throw insight::ExternalProcessFailed(
            retcode, "setSet", boost::join(errout, "\n ") );
  }
}

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap)
{
  std::vector<std::string> args;
  if (noFlipMap) args.push_back("-noFlipMap");
  ofc.executeCommand(location, "setsToZones", args);
}

void convertPatchPairToCyclic
(
  const OpenFOAMCase& ofc,
  const boost::filesystem::path& location, 
  const std::string& namePrefix
)
{
  using namespace createPatchOps;
  std::vector<createPatchOperatorPtr> ops;
  
  createCyclicOperator::Parameters p;
  p
     .set_patches_half1( { namePrefix+"_half1" } )
     .set_name(namePrefix)
     .set_patches( { namePrefix+"_half0" } )
      ;
  ops.push_back(createPatchOperatorPtr(new createCyclicOperator(p) ) );
  
  createPatch(ofc, location, ops, true);
}

arma::mat matchValue(const std::string& vs)
{
  boost::regex re_vector ("^\\(([^ ]+) ([^ ]+) ([^ ]+)\\)$");
  try
  {
      arma::mat d = {
          toNumber<double> ( vs )
      };
      return d;
  }
  catch (...)
  {
      boost::match_results<std::string::const_iterator> w;
      if (!boost::regex_match ( vs, w, re_vector))
          throw insight::Exception("reported value of patch integral was neither scalar nor vector!");
      arma::mat d = {
          toNumber<double> ( w[1] ),
          toNumber<double> ( w[2] ),
          toNumber<double> ( w[3] )
      };
      return d;
  }
  return arma::mat();
}

patchIntegrate::patchIntegrate
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    const std::string& fieldName,
    const std::string& patchNamePattern,
    const std::string& regionName,
    const std::vector<std::string>& addopts
)
{
  boost::regex pat ( patchNamePattern );

  // get all patch name candidates
  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict ( location, boundaryDict, regionName );

  std::vector<std::string> patches;
  for ( const OFDictData::dict::value_type& de: boundaryDict )
  {
    if ( regex_match ( de.first, pat ) )
      {
        patches.push_back ( de.first );
        break;
      }
  }

  int ncomp=1;
  arma::mat result;
  for ( const std::string& patchName: patches )
  {
    std::vector<std::string> opts;
    copy ( addopts.begin(), addopts.end(), back_inserter ( opts ) );

    if (!regionName.empty())
    {
        opts.push_back("-region");
        opts.push_back ( regionName );
    }

    std::vector<std::string> output;
    if (cm.OFversion()<400)
    {
        opts.push_back ( fieldName );
        opts.push_back ( patchName );
        cm.executeCommand ( location, "patchIntegrate", opts, &output );
    }
    else
    {
        opts.insert(opts.begin(), 
            boost::str( boost::format("patchIntegrate(name=%s,%s)") % patchName % fieldName )
        );
        opts.insert(opts.begin(), "-func");
        cm.executeCommand ( location, "postProcess", opts, &output );
    }

    boost::regex 
        re_time ( "^ *Time = (.+)$" ),
        re_mag_sum ( "^ *Integral of (.+) over patch (.+)\\[(.+)\\] = (.+)$" ),
        re_mag_int ( "^ *Integral of (.+) over area magnitude of patch (.+)\\[(.+)\\] = (.+)$" ),
        re_mag_int4 ( "^ *areaIntegrate\\((.+)\\) of (.+) = (.+)$" ),
        re_area ( "^ *Area magnitude of patch (.+)\\[(.+)\\] = (.+)$" ),
        re_area4 ( "^ *total area *= (.+)$" )
        ;

    
    boost::match_results<std::string::const_iterator> what;
    double time=0;
    std::vector<double> times, areadata;
    std::vector<arma::mat> data;
    for ( const std::string& line: output )
    {
      if ( boost::regex_match ( line, what, re_time ) )
        {
           time=toNumber<double> ( what[1] );
          times.push_back ( time );
          if ( times.size()-areadata.size()>1 ) // area may be reported not for every time step
            {
              areadata.push_back(areadata.back());
            }
        }

      if ( boost::regex_match ( line, what, re_mag_int ) )
        {
           cout<<what[1]<<" : "<<what[4]<<endl;
           data.push_back(matchValue(what[4]));
        }
      if ( boost::regex_match ( line, what, re_mag_int4 ) )
        {
          cout<<what[1]<<" : "<<what[3]<<endl;
          data.push_back(matchValue(what[3]));
        }
      else if ( boost::regex_match ( line, what, re_mag_sum ) )
        {
           cout<<what[1]<<" : "<<what[4]<<endl;
           data.push_back(matchValue(what[4]));
        }

      if ( boost::regex_match ( line, what, re_area ) )
        {
           cout<<what[1]<<" : "<<what[3]<<endl;
           areadata.push_back ( toNumber<double> ( what[3] ) );
        }
      if ( boost::regex_match ( line, what, re_area4 ) )
        {
          cout<<" Area : "<<what[1]<<endl;
          areadata.push_back ( toNumber<double> ( what[1] ) );
        }
    }
    if ( times.size()-areadata.size()==1 ) // area may not have been reported not for every time step
      {
        areadata.push_back(areadata.back());
      }

    if ( ( data.size() !=areadata.size() ) || ( data.size() !=times.size() ) )
      throw insight::Exception ( boost::str(boost::format(
          "Inconsistent information returned by patchIntegrate: number of values (%d) not equal to number of areas (%d) and number of times (%d)."
          ) % data.size() % areadata.size() % times.size() ) );

    if (data.size()>0)
      {
        ncomp=data[0].n_cols;
        arma::mat res=zeros ( data.size(), 2+ncomp );
        for ( size_t i=0; i<data.size(); i++ )
          {
            res ( i,0 ) =times[i];
            res ( i,1 ) =areadata[i];

            for (int j=0; j<ncomp; j++)
              {
                res ( i, j+2 ) =data[i](j);
              }
          }

        if ( result.n_cols==0 && result.n_rows==0 )
          result=res;
        else
          result+=res;
      }
  }

  if (result.n_cols>0)
    {
      t_ = result.col(0);
      A_ = result.col(1);
      integral_values_ = result.cols(2,2+ncomp-1);
    }
}

size_t patchIntegrate::n() const
{
  return t_.n_rows;
}

patchArea::patchArea(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    const std::string& patchName)
{
    std::shared_ptr<std::vector<boost::filesystem::path> > files;
    if (!boost::filesystem::exists(location/"system"/"controlDict"))
    {
        // might be, there is only the mesh yet and no case config
        OpenFOAMCase dummy(cm.ofe());
        dummy.insert(std::make_unique<MeshingNumerics>(dummy));
        // ensure there is a controlDict for patchArea later
        files=std::make_shared<std::vector<boost::filesystem::path> >(
            std::vector<boost::filesystem::path>
            {
             location/"system"/"controlDict",
             location/"system"/"fvSolution",
             location/"system"/"fvSchemes"
            });
        // dir needs to exist for file equivalent check to work
        boost::filesystem::create_directories(location/"system");
        dummy.createOnDisk(location, files);
    }
  std::vector<std::string> output;
  cm.executeCommand ( location, "patchArea", {patchName}, &output );

  //cleanup, if required
  if (files)
  {
      for (auto f: *files)
      {
          boost::filesystem::remove(f);
      }
  }

  boost::regex
      re_total ( "^TOTAL A=(.+) normal=\\((.+) (.+) (.+)\\) ctr=\\((.+) (.+) (.+)\\)$" )
      ;


  boost::match_results<std::string::const_iterator> what;
  for ( const std::string& line: output )
  {
    if ( boost::regex_match ( line, what, re_total ) )
      {
        A_=toNumber<double> ( what[1] );
        n_=vec3(
              toNumber<double> ( what[2] ),
            toNumber<double> ( what[3] ),
            toNumber<double> ( what[4] )
            );
        ctr_=vec3(
              toNumber<double> ( what[5] ),
            toNumber<double> ( what[6] ),
            toNumber<double> ( what[7] )
            );
        return;
      }
  }

  throw insight::Exception("patchArea: there was no interpretable output when searching for area and normal pf patch "+patchName+".");
}

arma::mat projectedArea
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& direction,
  const std::vector<std::string>& patches,
  const std::vector<std::string>& addopts
)
{
  std::vector<std::string> opts;
  std::string pl="( ";
  for (const std::string& pn: patches)
  {
    pl+=pn+" ";
  }
  pl+=")";
  opts.push_back(pl);
  opts.push_back(OFDictData::toString(OFDictData::vector3(direction)));
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  
  std::vector<std::string> output;
  cm.executeCommand(location, "projectedArea", opts, &output);

  std::vector<double> t, A;
  boost::regex re_area("^Projected area at time (.+) = (.+)$");
  for (const std::string & line: output)
  {
    boost::match_results<std::string::const_iterator> what;
    if (boost::regex_match(line, what, re_area))
    {
      t.push_back(toNumber<double>(what[1]));
      A.push_back(toNumber<double>(what[2]));
    }
  }
  
  return arma::mat( join_rows( arma::mat(t.data(), t.size(), 1), arma::mat(A.data(), A.size(), 1) ) );
}

arma::mat minPatchPressure
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string& patch,
  const double& Af,
  const std::vector<std::string>& addopts
)
{
  CurrentExceptionContext ex("computing minimum pressure on patch "+patch+" in case \""+location.string()+"\"");

  std::vector<std::string> opts;
  opts.push_back(patch);
  opts.push_back(str(format("%g") % Af));
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  
  std::vector<std::string> output;
  cm.executeCommand(location, "minPatchPressure", opts, &output);

  std::vector<double> t, minp;
  boost::regex re("^Minimum pressure at t=(.+) pmin=(.+)$");
  for (const std::string & line: output)
  {
    boost::match_results<std::string::const_iterator> what;
    if (boost::regex_match(line, what, re))
    {
      try {
        t.push_back(toNumber<double>(what[1]));
      } catch (const boost::bad_lexical_cast& e) {
        throw insight::Exception("expected a number, got \""+what[1]+"\"");
      }

      try {
        minp.push_back(toNumber<double>(what[2]));
      } catch (const boost::bad_lexical_cast& e) {
        throw insight::Exception("expected a number, got \""+what[2]+"\"");
      }
    }
  }
  
  return arma::mat( join_rows( arma::mat(t.data(), t.size(), 1), arma::mat(minp.data(), minp.size(), 1) ) );
}

void createBaffles
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location, 
  const std::string& faceZoneName
)
{
  OFDictData::dictFile cbd;
  cbd["internalFacesOnly"]=true;
  
  OFDictData::dict bsd;
  bsd["type"]="faceZone";
  bsd["zoneName"]=faceZoneName;
  
  OFDictData::dict ppd;
  ppd["type"]="wall";
  bsd["patchPairs"]=ppd;
  
  OFDictData::dict baffles;
  baffles[faceZoneName]=bsd;
  
  cbd["baffles"]=baffles;
  
  cbd.write( location / "system" / "createBafflesDict" );

  std::vector<std::string> opt;
  cm.executeCommand(location, "createBaffles", opt);
}

std::pair<arma::mat, arma::mat> zoneExtrema
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string fieldName,
  const std::string zoneName,
  const std::vector<std::string>& addopts
)
{
  std::vector<std::string> opts;
  opts.push_back(fieldName);
  opts.push_back(zoneName);
  copy(addopts.begin(), addopts.end(), back_inserter(opts));

  std::vector<std::string> output;
  cm.executeCommand(location, "zoneExtrema", opts, &output);
  
  boost::regex re_vec("^@t=(.+) : min / max \\[(.+)\\]= (.+) (.+) (.+) / (.+) (.+) (.+)$");
  boost::match_results<std::string::const_iterator> what;
  
  arma::mat mi, ma;
  
  for (const std::string& l: output)
  {
    if (boost::regex_match(l, what, re_vec))
    {
      double t=toNumber<double>(what[1]);
      arma::mat mir, mar;
      mir = ArmaMatCmpts{
          { t, toNumber<double>(what[3]), toNumber<double>(what[4]), toNumber<double>(what[5]) }
      };
      mar = ArmaMatCmpts{
          {t, toNumber<double>(what[6]), toNumber<double>(what[7]), toNumber<double>(what[8]) }
      };
      if (mi.n_rows==0) mi=mir; else mi=join_cols(mi, mir);
      if (ma.n_rows==0) ma=mar; else ma=join_cols(ma, mar);
    }
  }
  
  cout<<mi<<ma<<endl;
  
  return std::pair<arma::mat,arma::mat>(mi, ma);
}

void removeCellSetFromMesh
(
  const OpenFOAMCase& cm,
  const path& location,
  const string& cellSetName
)
{
  std::vector<std::string> opts;
  opts.push_back(cellSetName);
  opts.push_back("-overwrite");

  cm.executeCommand(location, "subsetMesh", opts);
}

std::set<std::string>
readPatchNameList(
        const OpenFOAMCase& cm,
        const boost::filesystem::path &caseLocation,
        bool parallel,
        const std::string& regionName,
        const std::string& time )
{
    std::set<std::string> plist;
    boost::regex procDir("^processor.*");
    boost::regex toSkip("^procBoundary.*");

    auto insertPatches = [&](const OFDictData::dict& boundaryDict)
    {
        for (const auto& e: boundaryDict)
        {
            if (!boost::regex_match(e.first, toSkip))
                plist.insert(e.first);
        }
    };

    if (parallel)
    {
        for (boost::filesystem::directory_iterator i(caseLocation);
             i!=boost::filesystem::directory_iterator(); ++i)
        {
            if (boost::regex_match(i->path().filename().string(), procDir))
            {
                OFDictData::dict boundaryDict;
                cm.parseBoundaryDict(i->path(), boundaryDict, regionName, time);
                insertPatches(boundaryDict);
            }
        }
    }
    else
    {
        OFDictData::dict boundaryDict;
        cm.parseBoundaryDict(caseLocation, boundaryDict, regionName, time);
        insertPatches(boundaryDict);
    }
    return plist;
}

PatchLayers::PatchLayers()
{}

PatchLayers::PatchLayers(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& caseLocation,
        bool parallel,
        const std::string& regionName,
        const std::string& time )
{
    auto patches = readPatchNameList(cm, caseLocation, parallel);
    for (const auto& p: patches)
    {
        insert({p, 0});
        std::cout<<"dir="<<caseLocation<<", patch="<<p<<std::endl;
    }
}

void PatchLayers::setByPattern(const std::string& regex_pattern, int nLayers)
{
    boost::regex pattern(regex_pattern);

    for (auto& pi: *this)
    {
        if (boost::regex_match(pi.first, pattern))
            (*this)[pi.first]=nLayers;
    }
}

arma::mat surfaceProjectLine
(
 const OFEnvironment& ofe, 
 const path& surfaceFile, 
 const arma::mat& start, 
 const arma::mat& end, 
 int npts, 
 const arma::mat& projdir)
{
  std::vector<std::string> opts;
  opts.push_back(surfaceFile.string());
  opts.push_back(OFDictData::toString(OFDictData::vector3(start)));
  opts.push_back(OFDictData::toString(OFDictData::vector3(end)));
  opts.push_back(toString(npts));
  opts.push_back(OFDictData::toString(OFDictData::vector3(projdir)));
//   copy(addopts.begin(), addopts.end(), back_inserter(opts));

  std::vector<std::string> output;
  OpenFOAMCase(ofe).executeCommand(surfaceFile.parent_path(), "surfaceProjectLine", opts, &output);
  
  boost::regex re_res("curve = \\((.*)\\)$");
  boost::match_results<std::string::const_iterator> what;
  
  for (const std::string& l: output)
  {
    if (boost::regex_match(l, what, re_res))
    {
      std::vector<double> data;
      std::vector<std::string> pairs;
      std::string matched(what[1]);
      boost::split(pairs, matched, boost::is_any_of(","));
      for (const std::string& p: pairs)
      {
	std::istringstream is(p);
	double x, r;
	is>>x>>r;
	data.push_back(x);
	data.push_back(r);
      }
      arma::mat result(data.data(), 2, data.size()/2);
      return result.t();
    }
  }
  
  throw insight::Exception("could not extract coordinate points!");
  
  return arma::mat();
}

 

std::vector<std::string> patchList
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseDir,
    const std::string& include,
    const std::vector<std::string>& exclude
)
{
  std::vector<std::string> result;
  
  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(caseDir, boundaryDict);
  
  const boost::regex filter( include );
  
  for (const OFDictData::dict::value_type& patch: boundaryDict)
  {
      std::string patchname = patch.first;
      
      boost::smatch what;
      if (boost::regex_match( patchname, what, filter ))
      {
          bool excl=false;
          for (std::string expat: exclude)
          {
              std::cout<<" ++ include patch "<<patchname<<" because of regex_rule "<<include<<std::endl;
              if (expat[0]=='\"')
              {
                  expat.erase( 0, 1 ); // erase the first character
                  expat.erase( expat.size() - 1 ); // erase the last character
                  if (boost::regex_match( patchname, what, boost::regex(expat) )) 
                    { 
                        std::cout<<"  -- exclude patch "<<patchname<<" because of regex_rule "<<expat<<std::endl;
                        excl=true; 
                        break; 
                    }
              }
              else 
              { 
                  if (patchname==expat) 
                    { 
                        std::cout<<"  -- exclude patch "<<patchname<<" because of direct match."<<std::endl;
                        excl=true; 
                        break; 
                    }
              }
          }
          if (!excl) result.push_back(patchname);  
      } else
      {
          std::cout<<" no match for patch "<<patchname<<" for regex_rule "<<include<<std::endl;
      }
  }
  
  return result;
}


}
