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

#include <iostream>

#include "base/exception.h"
#include "base/analysis.h"
#include "base/boost_include.h"
#include "base/outputanalyzer.h"
#include "openfoam/blockmeshoutputanalyzer.h"

#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamdict.h"

#include "openfoam/caseelements/boundarycondition.h"
#include "openfoam/caseelements/boundaryconditions/cyclicpairbc.h"
#include "openfoam/caseelements/numerics/fvnumerics.h"
//#include "openfoam/caseelements/basiccaseelements.h"
//#include "openfoam/caseelements/numerics/basicnumericscaseelements.h"


#include <boost/asio.hpp>
#include <boost/process/async.hpp>


using namespace std;
using namespace boost;
using namespace boost::filesystem;
namespace fs = boost::filesystem;
using namespace insight::OFDictData;


namespace insight
{




OFDictData::dict OpenFOAMCase::diagonalSolverSetup() const
{
  OFDictData::dict d;
  d["solver"]="diagonal";
  return d;
}




OFDictData::dict OpenFOAMCase::stdAsymmSolverSetup(double tol, double reltol, int minIter, const std::string& preCon) const
{
  OFDictData::dict d;
  if (OFversion()<170)
    d["solver"]="BiCGStab";
  else
    d["solver"]="PBiCGStab";
  d["preconditioner"]=preCon;
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  if (minIter) d["minIter"]=minIter;
  return d;
}




OFDictData::dict OpenFOAMCase::stdSymmSolverSetup(double tol, double reltol, int maxIter) const
{
  OFDictData::dict d;
  d["solver"]="PCG";
  d["preconditioner"]="DIC";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["maxIter"]=maxIter;
  return d;
}




OFDictData::dict OpenFOAMCase::GAMGSolverSetup(double tol, double reltol) const
{
  OFDictData::dict d;
  d["solver"]="GAMG";
  d["minIter"]=1;
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["smoother"]="DICGaussSeidel";
  d["nPreSweeps"]=2;
  d["nPostSweeps"]=2;
  d["cacheAgglomeration"]="on";
  d["agglomerator"]="faceAreaPair";
  d["nCellsInCoarsestLevel"]=100;
  d["mergeLevels"]=1;
  return d;
}




OFDictData::dict OpenFOAMCase::GAMGPCGSolverSetup(double tol, double reltol) const
{
  OFDictData::dict d;
  d["solver"]="PCG";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["minIter"]=1;
  OFDictData::dict pd;
  pd["preconditioner"]="GAMG";
  pd["smoother"]="DICGaussSeidel";
  pd["nPreSweeps"]=2;
  pd["nPostSweeps"]=2;
  pd["cacheAgglomeration"]="on";
  pd["agglomerator"]="faceAreaPair";
  pd["nCellsInCoarsestLevel"]=100;
  pd["mergeLevels"]=1;
  d["preconditioner"]=pd;
  return d;
}




OFDictData::dict OpenFOAMCase::smoothSolverSetup(double tol, double reltol, int minIter) const
{
  OFDictData::dict d;
  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["nSweeps"]=1;
  if (minIter) d["minIter"]=minIter;
  return d;
}











const OFDictData::dimensionSet dimPressure = OFDictData::dimension(1, -1, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinPressure = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinEnergy = OFDictData::dimension(0, 2, -2, 0, 0, 0, 0);
const OFDictData::dimensionSet dimVelocity = OFDictData::dimension(0, 1, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimLength = OFDictData::dimension(0, 1, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimDensity = OFDictData::dimension(1, -3, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimless = OFDictData::dimension(0, 0, 0, 0, 0, 0, 0);
const OFDictData::dimensionSet dimKinViscosity = OFDictData::dimension(0, 2, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimDynViscosity = OFDictData::dimension(1, -1, -1, 0, 0, 0, 0);
const OFDictData::dimensionSet dimTemperature = OFDictData::dimension(0, 0, 0, 1, 0, 0, 0);
const OFDictData::dimensionSet dimCurrent = OFDictData::dimension(0, 0, 0, 0, 0, 1, 0);




bool OpenFOAMCase::isCompressible() const
{
  return findUniqueElement<FVNumerics>().isCompressible();
}




bool OpenFOAMCase::hasCyclicBC() const
{
  std::set<CyclicPairBC*> cyclics = const_cast<OpenFOAMCase*>(this)->findElements<CyclicPairBC>();
  return (cyclics.size()>0);
}




void OpenFOAMCase::modifyFilesOnDiskBeforeDictCreation ( const boost::filesystem::path& location ) const
{
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
  {
    const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
    if (e)
    {
      e->modifyFilesOnDiskBeforeDictCreation(*this, location);
    }
  }
}




std::string modifyPathForRegion(const std::string& orgPath, const std::string& regionName)
{
  fs::path dpath(orgPath);

  cout<<dpath<< " ==>> ";

  if (dpath.is_relative())
  {
    std::vector<fs::path> pathcmpts;
    std::copy(dpath.begin(), dpath.end(), std::back_inserter(pathcmpts));
    if (
        (pathcmpts[0]=="constant")
        ||
        (pathcmpts[0]=="system")
        ||
        isNumber(pathcmpts[0].string())
        )
    {
      pathcmpts.insert( ++pathcmpts.begin(), regionName );
    }

    dpath = pathcmpts[0];
    for (auto i = ++pathcmpts.begin(); i!=pathcmpts.end(); ++i)
    {
      dpath /= *i;
    }
  }

  cout << dpath << endl;

  return dpath.string();
}




std::shared_ptr<OFdicts> OpenFOAMCase::createDictionaries() const
{
  std::shared_ptr<OFdicts> dictionaries(new OFdicts);

  // populate fields list
  createFieldListIfRequired();
  
  // create field dictionaries first
  for ( const FieldList::value_type& i: fields_)
  {
    OFDictData::dictFile& field = dictionaries->addFieldIfNonexistent("0/"+i.first, i.second);
    
    const dimensionSet& dimset=boost::fusion::get<1>(i.second);
    std::ostringstream dimss; 
    //dimss << dimset;
    dimss <<"[ "; for (int c: dimset) dimss <<c<<" "; dimss<<"]";
    field["dimensions"] = OFDictData::data( dimss.str() );
    
    std::string vstr="";
    const FieldValue& val = boost::fusion::get<2>(i.second);
    for ( const double& v: val)
    {
      vstr+=" "+lexical_cast<std::string>(v);
    }
    if (val.size()>1) vstr  = " ("+vstr+" )";
    
    field["internalField"] = OFDictData::data( "uniform"+vstr );
    field["boundaryField"] = OFDictData::dict();
  }
  
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
       i!=elements_.end(); i++)
       {
	 const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
	 if (e)
	 {
	   e->addIntoDictionaries(*dictionaries);
	 }
       }  

  // Include all dicts belonging to regions. Modify their path accordingly.
  for (const auto& region: regions_)
  {
    std::shared_ptr<OFdicts> rd = region.second->createDictionaries();

    while (rd->size())
    {
      auto dictName = rd->begin()->first;
      auto dict = rd->release(rd->begin());
      auto newPath=modifyPathForRegion(dictName, region.first);
      dictionaries->insert(newPath, dict.release());
    }
  }

  return dictionaries;
}




void OpenFOAMCase::modifyMeshOnDisk(const boost::filesystem::path& location) const
{
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
  {
    const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
    if (e)
    {
      e->modifyMeshOnDisk(*this, location);
    }
  }
}




void OpenFOAMCase::modifyCaseOnDisk(const boost::filesystem::path& location) const
{
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
  {
    const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
    if (e)
    {
      e->modifyCaseOnDisk(*this, location);
    }
  }
}




void OpenFOAMCase::createOnDisk
(
    const boost::filesystem::path& location, 
    const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles
)
{

  if (!restrictToFiles)
    modifyFilesOnDiskBeforeDictCreation( location );

  std::shared_ptr<OFdicts> dictionaries=createDictionaries();
  createOnDisk(location, dictionaries, restrictToFiles);
}




void OpenFOAMCase::createOnDisk
(
    const boost::filesystem::path& location, 
    std::shared_ptr<OFdicts> dictionaries, 
    const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles
)
{
  boost::filesystem::path basepath(location);

  for (OFdicts::const_iterator i=dictionaries->begin();
      i!=dictionaries->end(); i++)
  {
    boost::filesystem::path dictpath = basepath / i->first;
    std::cout<<"FILE "<<dictpath<<": ";
    
    bool ok_to_create=true;
    
    if (restrictToFiles)
    {
        ok_to_create=false;
        for (const boost::filesystem::path& fp: *restrictToFiles)
        {
            if ( boost::filesystem::equivalent(dictpath, fp) ) ok_to_create=true;
        }
    }
    
    if (ok_to_create)
    {
        if (!exists(dictpath.parent_path())) 
        {
          boost::filesystem::create_directories(dictpath.parent_path());
        }
        
        {
          std::ofstream f(dictpath.c_str());
          i->second->write(dictpath);
        }
        std::cout<<"CREATED."<<std::endl;
    } else
    {
        std::cout<<"SKIPPED."<<std::endl;
    }
  }

  for (auto& af: dictionaries->additionalInputFiles_)
  {
    af.first->copyTo( basepath / af.second );
  }
}




bool OpenFOAMCase::meshPresentOnDisk( const boost::filesystem::path& location ) const
{
  path meshPath = location/"constant"/"polyMesh";
  return
    exists(meshPath) &&
    ( exists(meshPath/"points") || exists(meshPath/"points.gz") ) &&
    ( exists(meshPath/"faces") || exists(meshPath/"faces.gz") ) &&
    ( exists(meshPath/"neighbour") || exists(meshPath/"neighbour.gz") ) &&
    ( exists(meshPath/"owner") || exists(meshPath/"owner.gz") ) &&
    exists(meshPath/"boundary");
}




bool OpenFOAMCase::outputTimesPresentOnDisk( const boost::filesystem::path& location, bool checkpar ) const
{

  OFDictData::dict controlDict;
  try
  {
    readOpenFOAMDict(location/"system"/"controlDict", controlDict);
  }
  catch (const std::exception&)
  {
    return false;
  }

  double startTime = controlDict.getDoubleOrInt("startTime");
  
  TimeDirectoryList timedirs;
  if (checkpar)
  {
    boost::filesystem::path pd=location/"processor0";
    if (!boost::filesystem::exists(pd)) return false;
    timedirs=listTimeDirectories(pd);
  }
  else
    timedirs=listTimeDirectories(location);
  
  if (timedirs.size()<1)
      return false;
  
  if (fabs(timedirs.rbegin()->first - startTime)<1e-10)
      return false;
  else
      return true;
//   return (timedirs.size()>1);
}




void OpenFOAMCase::removeProcessorDirectories( const boost::filesystem::path& location ) const
{
  boost::regex re_pdn("^processor([0-9]+)$");
  std::vector<path> list;
  if ( exists( location ) ) 
  {
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( location );
          itr != end_itr;
          ++itr )
    {
      if ( is_directory(itr->status()) )
      {
	boost::match_results<std::string::const_iterator> what;
        std::string fn=itr->path().filename().string();
	if (boost::regex_match(fn, what, re_pdn))
	  remove_all(itr->path());
      }
    }
  }  
}




void OpenFOAMCase::setFromXML(const std::string& contents, const boost::filesystem::path& file, bool skipOFE, bool skipBCs, const boost::filesystem::path& casepath)
{
  using namespace rapidxml;
  
  xml_document<> doc;
  doc.parse<0> ( const_cast<char*>(&contents[0]) );

  xml_node<> *rootnode = doc.first_node ( "root" );

  if (!skipOFE)
  {
    xml_node<> *OFEnode = rootnode->first_node ( "OFE" );
    if ( OFEnode )
      {
        std::string name = OFEnode->first_attribute ( "name" )->value();
        env_=insight::OFEs::get(name.c_str());
      }
  }

  for ( xml_node<> *e = rootnode->first_node ( "OpenFOAMCaseElement" ); e; e = e->next_sibling ( "OpenFOAMCaseElement" ) )
    {
      std::string type_name = e->first_attribute ( "type" )->value();

      ParameterSet cp = OpenFOAMCaseElement::defaultParameters(type_name);
      cp.readFromNode ( doc, *e, file.parent_path() );
      this->insert(OpenFOAMCaseElement::lookup(type_name, *this, cp));
    }

  if ( !skipBCs )
    {
        insight::OFDictData::dict boundaryDict;
        
      xml_node<> *BCnode = rootnode->first_node ( "BoundaryConditions" );
      if ( BCnode )
        {
          bool bdp=true;
          try
            {
              // parse boundary information
              this->parseBoundaryDict ( casepath, boundaryDict );
            }
          catch ( ... )
            {
              bdp=false;
            }

          if ( bdp )
            {
              xml_node<> *unassignedBCnode = BCnode->first_node ( "UnassignedPatches" );
              std::string def_bc_type = unassignedBCnode->first_attribute ( "BCtype" )->value();
              ParameterSet defp;
              if ( def_bc_type!="" )
                {
                  defp = BoundaryCondition::defaultParameters ( def_bc_type );
                }

              for ( xml_node<> *e = BCnode->first_node ( "Patch" ); e; e = e->next_sibling ( "Patch" ) )
                {
                  std::string patch_name = e->first_attribute ( "patchName" )->value();
                  std::string bc_type = e->first_attribute ( "BCtype" )->value();
                  if ( bc_type!="" )
                    {
                      ParameterSet curp = BoundaryCondition::defaultParameters ( bc_type );
                      curp.readFromNode ( doc, *e, file.parent_path() );
                      this->insert ( insight::BoundaryCondition::lookup ( bc_type, *this, patch_name, boundaryDict, curp ) );
                    }
                }
              if ( def_bc_type!="" )
                {
                  this->addRemainingBCs ( def_bc_type, boundaryDict, defp );
                }
            }
          else
            {
              insight::Warning("The boundary file could not be parsed! Skipping BC configuration." );
            }
        }
    }
}




void OpenFOAMCase::loadFromFile(const boost::filesystem::path& filename, bool skipOFE, bool skipBCs, const boost::filesystem::path& casepath)
{
    if (!boost::filesystem::exists(filename))
        throw insight::Exception("Input file "+filename.string()+" does not exist!");
    
    std::string contents;
    readFileIntoString(filename, contents);
    
    setFromXML(contents, filename, skipOFE, skipBCs, casepath);
}




std::vector< string > OpenFOAMCase::fieldNames() const
{
    createFieldListIfRequired();
  std::vector<std::string> fns;
  for (const FieldList::value_type& v: fields_)
  {
    fns.push_back(v.first);
  }
  return fns;
}




bool OpenFOAMCase::hasField(const std::string& fname ) const
{
  createFieldListIfRequired();
  return fields_.find ( fname ) != fields_.end();
}




bool OpenFOAMCase::hasPrghPressureField() const
{
  bool has=false;
  for (const auto& fn: std::vector<std::string>({"pd", "p_gh", "p_rgh"}) )
  {
    has = has || hasField(fn);
  }

  return has;
}




void OpenFOAMCase::createFieldListIfRequired() const
{
  if ( !fieldListCompleted_ )
    {
//      FieldList& fields = const_cast<FieldList&> ( fields_ );

      for ( boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
            i!=elements_.end(); i++ )
        {
          const OpenFOAMCaseElement *e= dynamic_cast<const OpenFOAMCaseElement*> ( & ( *i ) );
          if ( e )
            {
              e->addFields ( const_cast<OpenFOAMCase&> ( *this ) );
            }
        }
      const_cast<bool&>(fieldListCompleted_)=true;
    }
}




OpenFOAMCase::OpenFOAMCase(const OFEnvironment& env)
: Case(),
  env_(env),
  requiredMapMethod_(directMapMethod)
{
}




OpenFOAMCase::OpenFOAMCase(const OpenFOAMCase& other)
: Case(other),
  env_(other.env_),
  requiredMapMethod_(other.requiredMapMethod_)
{
}




OpenFOAMCase::~OpenFOAMCase()
{
}




void OpenFOAMCase::addRegionCase(const std::string& regionName, std::shared_ptr<OpenFOAMCase> regionCase)
{
  insight::assertion(
        regionCase->OFversion()==OFversion(),
        "the selected OpenFOAM versions have to be identical"
        );

  regions_.emplace(regionName, regionCase);
}




void OpenFOAMCase::addField(const std::string& name, const FieldInfo& field)
{
    if (fieldListCompleted_)
        throw insight::Exception("Internal error: tryed to add field "+name+" after field list was already build!");
    
  fields_[name]=field;
}




boost::filesystem::path OpenFOAMCase::boundaryDictPath(const boost::filesystem::path& location, const std::string& regionName, const std::string& time) const
{
  boost::filesystem::path basepath(location);
  if (regionName.empty())
    return basepath / time / "polyMesh" / "boundary";
  else
    return basepath / time / regionName / "polyMesh" / "boundary";
}




void OpenFOAMCase::parseBoundaryDict(const boost::filesystem::path& location, OFDictData::dict& boundaryDict, const std::string& regionName, const std::string& time) const
{
  boost::filesystem::path dictpath = boundaryDictPath(location, regionName, time);
  std::ifstream f(dictpath.c_str());
  if (!readOpenFOAMBoundaryDict(f, boundaryDict))
      throw insight::Exception("Failed to parse boundary dict "+dictpath.string());
}




std::string OpenFOAMCase::cmdString
(
  const boost::filesystem::path& location, 
  const std::string& cmd,
  std::vector<std::string> argv
)
const
{
  std::string shellcmd="";
  
  shellcmd += 
    "source "+env_.bashrc().string()+";"
    "cd \""+boost::filesystem::absolute(location).string()+"\";"
    + cmd;
  for (std::string& arg: argv)
  {
    shellcmd+=" \""+escapeShellSymbols(arg)+"\"";
  }

  return shellcmd;
}


std::string mpirunCommand(int np)
{
    std::string execmd="mpirun -np "+lexical_cast<string>(np);

    std::string envvarname="INSIGHT_ADDITIONAL_MPIRUN_ARGS";
    if ( char *addargs=getenv ( envvarname.c_str() ) )
    {
        execmd += " "+std::string(addargs);
    }
    return execmd;
}


void OpenFOAMCase::executeCommand
(
  const boost::filesystem::path& location, 
  const std::string& cmd,
  std::vector<std::string> argv,
  std::vector<std::string>* output,
  int np,
  std::string *ovr_machine
) const
{
  string execmd=cmd;
  if (np>1)
  {
    execmd = mpirunCommand(np)+" "+cmd;
    argv.push_back("-parallel");
  }
  
  env_.executeCommand( cmdString(location, execmd, argv), { }, output, ovr_machine );
}




void OpenFOAMCase::runSolver
(
  const boost::filesystem::path& location, 
  OutputAnalyzer& analyzer,
  std::string cmd,
  int np,
  const std::vector<std::string>& addopts
) const
{
  string execmd=cmd;
  std::vector<std::string> argv;
  if (np>1)
  {
    execmd=mpirunCommand(np)+" "+cmd;
    argv.push_back("-parallel");
  }
  std::copy(addopts.begin(), addopts.end(), back_inserter(argv));



  auto job = env_.forkCommand( cmdString(location, execmd, argv) );


  job->ios_run_with_interruption(

        [&](const std::string& line)
        {
          cout<<line<<endl; // mirror to console
          analyzer.update(line);

          if (analyzer.stopRun())
          {
            std::ofstream f( (location/"wnowandstop").string() );
            f<<"STOP"<<std::endl;
          }
        },

        [&](const std::string& line)
        {
          // mirror to console
          cout<<"[E] "<<line<<endl; // mirror to console
          analyzer.update("[E] "+line);
        }
  );

  job->wait();

  if (job->process().exit_code()!=0)
      throw insight::Exception("OpenFOAMCase::runSolver(): external command execution failed with nonzero exit code!");
}




std::set<std::string> OpenFOAMCase::getUnhandledPatches(OFDictData::dict& boundaryDict) const
{
  typedef std::set<std::string> StringSet;
  StringSet unhandledPatches;
  for (const OFDictData::dict::value_type& bde: boundaryDict)
  {
    unhandledPatches.insert(bde.first);
  }
  
  for (boost::ptr_vector<CaseElement>::const_iterator i=elements_.begin();
      i!=elements_.end(); i++)
      {
	const OpenFOAMCaseElement* e= dynamic_cast<const OpenFOAMCaseElement*>(&(*i));
	for (const OFDictData::dict::value_type& bde: boundaryDict)
	{
	  std::string pn(bde.first);
	  
	  if (e->providesBCsForPatch(pn))
	  {
	    StringSet::iterator i=unhandledPatches.find(pn);
	    if (i!=unhandledPatches.end()) unhandledPatches.erase(i);
	  }
	}

      }
      
   return unhandledPatches;
}




JobPtr OpenFOAMCase::forkCommand
(
    const boost::filesystem::path& location,
    const std::string& cmd,
    std::vector<std::string> argv,
    std::string *ovr_machine
) const
{
    return env_.forkCommand ( cmdString ( location, cmd, argv ), { }, ovr_machine );
}




void OpenFOAMCase::runBlockMesh
(
    const boost::filesystem::path& location,
    int nBlocks,
    ProgressDisplayer* progressDisplayer
)
{
  BlockMeshOutputAnalyzer bma(progressDisplayer, nBlocks);
  runSolver(location, bma, "blockMesh");
}




void OpenFOAMCase::addRemainingBCs ( const std::string& bc_type, OFDictData::dict& boundaryDict, const ParameterSet& ps )
{
    typedef std::set<std::string> StringSet;
    StringSet unhandledPatches = getUnhandledPatches ( boundaryDict );

    for ( StringSet::const_iterator i=unhandledPatches.begin(); i!=unhandledPatches.end(); i++ ) {
        insert ( BoundaryCondition::lookup ( bc_type, *this, *i, boundaryDict, ps ) );
    }
}




std::vector<boost::filesystem::path> OpenFOAMCase::functionObjectOutputDirectories
(
    const boost::filesystem::path& caseLocation
) const
{
  path fp;
  std::shared_ptr<regex> filter;
  if ( OFversion() <170 )
  {
    fp=absolute ( caseLocation );
    filter.reset(new regex("(processor.*|constant|system|[0-9].*)"));
  }
  else
  {
    fp=absolute ( caseLocation ) / "postProcessing";
  }

  std::vector<boost::filesystem::path> results;

  for (directory_iterator it(fp);
       it!=directory_iterator(); ++it)
  {
    if (is_directory(it->status())) // if is directory
    {
      auto cp=it->path();
      if (!filter || (filter && !regex_search(cp.filename().string(), *filter))) // and not matching filter
      {
        if (listTimeDirectories(cp).size()>0) // and if contains time dirs
        {
          results.push_back(cp); // then add
        }
      }
    }
  }

  return results;
}




}
