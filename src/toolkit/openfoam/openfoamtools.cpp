/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "openfoamtools.h"
#include "openfoam/basiccaseelements.h"
#include "base/analysis.h"

#include "boost/filesystem.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/assign.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir)
{
  TimeDirectoryList list;
  if ( exists( dir ) ) 
  {
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( dir );
          itr != end_itr;
          ++itr )
    {
      if ( is_directory(itr->status()) )
      {
        std::string fn=itr->path().filename().string();
	try
	{
	  double time = lexical_cast<double>(fn);
	  list[time]=itr->path();
	}
	catch (...)
	{
	}
      }
    }
  }
  return list;
}

  
void setSet(const OpenFOAMCase& ofc, const boost::filesystem::path& location, const std::vector<std::string>& cmds)
{
  redi::opstream proc;
  
  std::vector<std::string> opts;
  if ((ofc.OFversion()>=220) && (listTimeDirectories(location).size()==0)) opts.push_back("-constant");
  ofc.forkCommand(proc, location, "setSet", opts);
  BOOST_FOREACH(const std::string& line, cmds)
  {
    proc << line << endl;
  }
  proc << "quit" << endl;
}

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap)
{
  std::vector<std::string> args;
  if (noFlipMap) args.push_back("-noFlipMap");
  ofc.executeCommand(location, "setsToZones", args);
}

void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify)
{
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.c_str();
  ::system(cmd.c_str());
  
  if (purify)
  {
    BOOST_FOREACH(const std::string& fname, 
		  list_of<std::string>("boundary")("faces")("neighbour")("owner")("points")
		  .convert_to_container<std::vector<std::string> >())
    {
      path gzname(fname.c_str()); gzname+=".gz";
      if (exists(source/gzname)) copy_file(source/gzname, target/gzname);
      else if (exists(source/fname)) copy_file(source/fname, target/fname);
      else throw insight::Exception("Essential mesh file "+fname+" not present in "+source.c_str());
    }
  }
  else
    throw insight::Exception("Not implemented!");
}

void create_symlink_force_overwrite(const path& source, const path& targ)
{
  if (is_symlink(targ))
    remove(targ);
  else
  {
    if (exists(targ))
      throw insight::Exception("Link target "+targ.string()+" exists and is not a symlink! Please remove manually first.");
  }
    
  create_symlink(source, targ);
}

void linkPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to)
{
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.c_str();
  ::system(cmd.c_str());
  
  BOOST_FOREACH(const std::string& fname, 
		list_of<std::string>("boundary")("faces")("neighbour")("owner")("points")
		.convert_to_container<std::vector<std::string> >())
  {
    path gzname(fname.c_str()); gzname+=".gz";
    if (exists(source/gzname)) create_symlink_force_overwrite(source/gzname, target/gzname);
    else if (exists(source/fname)) create_symlink_force_overwrite(source/fname, target/fname);
    else throw insight::Exception("Essential mesh file "+fname+" not present in "+source.c_str());
  }
}

void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to)
{
  if (!exists(to))
    create_directories(to);

  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( from );
	itr != end_itr;
	++itr )
  {
    if ( is_regular_file(itr->status()) )
    {
      copy_file(itr->path(), to/itr->path().filename());
    }
  }
}

namespace setFieldOps
{
 
setFieldOperator::setFieldOperator(Parameters const& p)
: p_(p)
{
}

fieldToCellOperator::fieldToCellOperator(Parameters const& p)
: setFieldOperator(p),
  p_(p)
{
}
  
void fieldToCellOperator::addIntoDictionary(OFDictData::dict& setFieldDict) const
{
  OFDictData::dict opdict;
  opdict["fieldName"]=p_.fieldName();
  opdict["min"]=p_.min();
  opdict["max"]=p_.max();

  OFDictData::list fve;
  BOOST_FOREACH(const FieldValueSpec& fvs, p_.fieldValues())
  {
    //std::ostringstream line;
    //line << fvs.get<0>() << " " << fvs.get<1>() ;
    fve.push_back( fvs );
  }
  opdict["fieldValues"]=fve;
  setFieldDict.getList("regions").push_back( fve );
  
}

setFieldOperator* fieldToCellOperator::clone() const
{
  return new fieldToCellOperator(p_);
}

}

void setFields(const OpenFOAMCase& ofc, const boost::filesystem::path& location, 
	       const std::vector<setFieldOps::FieldValueSpec>& defaultValues,
	       const boost::ptr_vector<setFieldOps::setFieldOperator>& ops)
{
  using namespace setFieldOps;
  
  OFDictData::dictFile setFieldsDict;
  
  OFDictData::list& dvl = setFieldsDict.addListIfNonexistent("defaultFieldValues");
  BOOST_FOREACH( const FieldValueSpec& dv, defaultValues)
  {
    dvl.push_back( dv );
  }
  
  setFieldsDict.addListIfNonexistent("regions");  
  BOOST_FOREACH( const setFieldOperator& op, ops)
  {
    op.addIntoDictionary(setFieldsDict);
  }
  
  // then write to file
  boost::filesystem::path dictpath = location / "system" / "setFieldsDict";
  if (!exists(dictpath.parent_path())) 
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }
  
  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, setFieldsDict, boost::filesystem::basename(dictpath));
  }

  ofc.executeCommand(location, "setFields");
}

namespace createPatchOps
{

createPatchOperator::createPatchOperator(Parameters const& p )
: p_(p)
{
}
  
void createPatchOperator::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const
{
  OFDictData::dict opdict;
  opdict["name"]=p_.name();
  opdict["constructFrom"]=p_.constructFrom();
  OFDictData::list pl;
  std::copy(p_.patches().begin(), p_.patches().end(), pl.begin());
  opdict["patches"]=pl;
  opdict["set"]=p_.set();

  OFDictData::dict opsubdict;
  opsubdict["type"]=p_.type();
  
  if (ofc.OFversion()<=160)
  {
    opdict["dictionary"]=opsubdict;
    createPatchDict.getList("patchInfo").push_back( opdict );
  }
  else
  {
    opdict["patchInfo"]=opsubdict;
    createPatchDict.getList("patches").push_back( opdict );
  }

}
  
createPatchOperator* createPatchOperator::clone() const
{
  return new createPatchOperator(p_);
}

createCyclicOperator::createCyclicOperator(Parameters const& p )
: p_(p)
{
}
  
void createCyclicOperator::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& createPatchDict) const
{
  std::vector<std::string> suffixes;
  if (ofc.OFversion()>=210)
  {
    suffixes.push_back("_half0");
    suffixes.push_back("_half1");
  }
  else
    suffixes.push_back("");
  
  BOOST_FOREACH(const std::string& suf, suffixes)
  {
    OFDictData::dict opdict;
    opdict["name"]=p_.name()+suf;
    opdict["constructFrom"]=p_.constructFrom();
    OFDictData::list pl;
    if (suf=="_half0" || suf=="")
    {
      pl.resize(pl.size()+p_.patches().size());
      std::copy(p_.patches().begin(), p_.patches().end(), pl.begin());
      opdict["set"]=p_.set();
    }
    if (suf=="_half1" || suf=="")
    {
      int osize=pl.size();
      pl.resize(osize+p_.patches_half1().size());
      std::copy(p_.patches_half1().begin(), p_.patches_half1().end(), pl.begin()+osize);
      if (suf!="") opdict["set"]=p_.set_half1();
    }
    opdict["patches"]=pl;

    OFDictData::dict opsubdict;
    opsubdict["type"]="cyclic";
    if (suf=="_half0") opsubdict["neighbourPatch"]=p_.name()+"_half1";
    if (suf=="_half1") opsubdict["neighbourPatch"]=p_.name()+"_half0";
    
    if (ofc.OFversion()>=210)
    {
      opdict["patchInfo"]=opsubdict;
      createPatchDict.getList("patches").push_back( opdict );
    }
    else
    {
      opdict["dictionary"]=opsubdict;
      createPatchDict.getList("patchInfo").push_back( opdict );
    }
  }

}
  
createPatchOperator* createCyclicOperator::clone() const
{
  return new createCyclicOperator(p_);
}

}

void createPatch(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location, 
		  const boost::ptr_vector<createPatchOps::createPatchOperator>& ops,
		  bool overwrite
		)
{
  using namespace createPatchOps;
  
  OFDictData::dictFile createPatchDict;
  
  createPatchDict["matchTolerance"] = 1e-3;
  createPatchDict["pointSync"] = false;
  
  if (ofc.OFversion()<=160)
    createPatchDict.addListIfNonexistent("patchInfo");  
  else
    createPatchDict.addListIfNonexistent("patches");  
  
  BOOST_FOREACH( const createPatchOperator& op, ops)
  {
    op.addIntoDictionary(ofc, createPatchDict);
  }
  
  // then write to file
  createPatchDict.write( location / "system" / "createPatchDict" );

  std::vector<std::string> opts;
  if (overwrite) opts.push_back("-overwrite");
    
  ofc.executeCommand(location, "createPatch", opts);
}

void convertPatchPairToCyclic
(
  const OpenFOAMCase& ofc,
  const boost::filesystem::path& location, 
  const std::string& namePrefix
)
{
  using namespace createPatchOps;
  boost::ptr_vector<createPatchOperator> ops;
  
  ops.push_back(new createCyclicOperator( createCyclicOperator::Parameters()
   .set_name(namePrefix)
   .set_patches( list_of<string>(namePrefix+"_half0") )
   .set_patches_half1( list_of<string>(namePrefix+"_half1") )
  ) );
  
  createPatch(ofc, location, ops, true);
}

void mergeMeshes(const OpenFOAMCase& targetcase, const boost::filesystem::path& source, const boost::filesystem::path& target)
{
  targetcase.executeCommand
  (
    target, "mergeMeshes", 
    list_of<std::string>
    (".")
    (boost::filesystem::absolute(source).c_str()) 
  );
}


void mapFields
(
  const OpenFOAMCase& targetcase, 
  const boost::filesystem::path& source, 
  const boost::filesystem::path& target,
  bool parallelTarget
)
{
  path mfdPath=target / "system" / "mapFieldsDict";
  if (!exists(mfdPath))
  {
    OFDictData::dictFile mapFieldsDict;
    mapFieldsDict["patchMap"] = OFDictData::list();
    mapFieldsDict["cuttingPatches"] = OFDictData::list();
    mapFieldsDict.write( mfdPath );
  }
  else
  {
    cout<<"A mapFieldsDict is existing. It will be used."<<endl;
  }

  std::vector<string> args =
    list_of<std::string>
    (boost::filesystem::absolute(source).c_str())
    ("-sourceTime")("latestTime")
    ;
  if (parallelTarget) 
    args.push_back("-parallelTarget");

  targetcase.executeCommand
  (
    target, "mapFields", args
  );
}


void resetMeshToLatestTimestep(const OpenFOAMCase& c, const boost::filesystem::path& location)
{
  TimeDirectoryList times = listTimeDirectories(boost::filesystem::absolute(location));
  boost::filesystem::path lastTime = times.rbegin()->second;
  
  remove_all(location/"constant"/"polyMesh");
  copyPolyMesh(lastTime, location/"constant", true);
  
  BOOST_FOREACH(const TimeDirectoryList::value_type& td, times)
  {
    remove_all(td.second);
  }
}

void runPotentialFoam
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  bool* stopFlagPtr,
  int np
)
{
  path fvSol(boost::filesystem::absolute(location)/"system"/"fvSolution");
  path fvSolBackup(fvSol); fvSolBackup.replace_extension(".potf");
  path fvSch(boost::filesystem::absolute(location)/"system"/"fvSchemes");
  path fvSchBackup(fvSch); fvSchBackup.replace_extension(".potf");
  
  if (exists(fvSol)) copy_file(fvSol, fvSolBackup, copy_option::overwrite_if_exists);
  if (exists(fvSch)) copy_file(fvSch, fvSchBackup, copy_option::overwrite_if_exists);
  
  OFDictData::dictFile fvSolution;
  OFDictData::dict& solvers=fvSolution.addSubDictIfNonexistent("solvers");
  solvers["p"]=stdSymmSolverSetup(1e-7, 0.01);
  fvSolution.addSubDictIfNonexistent("relaxationFactors");
  OFDictData::dict& potentialFlow=fvSolution.addSubDictIfNonexistent("potentialFlow");
  potentialFlow["nNonOrthogonalCorrectors"]=3;
  
  OFDictData::dictFile fvSchemes;
  fvSchemes.addSubDictIfNonexistent("ddtSchemes");
  fvSchemes.addSubDictIfNonexistent("gradSchemes");
  fvSchemes.addSubDictIfNonexistent("divSchemes");
  fvSchemes.addSubDictIfNonexistent("laplacianSchemes");
  fvSchemes.addSubDictIfNonexistent("interpolationSchemes");
  fvSchemes.addSubDictIfNonexistent("snGradSchemes");
  fvSchemes.addSubDictIfNonexistent("fluxRequired");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="Gauss upwind";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["p"]="";
  fluxRequired["default"]="no";
  
  // then write to file
  {
    std::ofstream f(fvSol.c_str());
    writeOpenFOAMDict(f, fvSolution, boost::filesystem::basename(fvSol));
    f.close();
  }
  {
    std::ofstream f(fvSch.c_str());
    writeOpenFOAMDict(f, fvSchemes, boost::filesystem::basename(fvSch));
    f.close();
  }
  
  TextProgressDisplayer displayer;
  SolverOutputAnalyzer analyzer(displayer);
  cm.runSolver(location, analyzer, "potentialFoam", stopFlagPtr, np);

  if (exists(fvSol)) copy_file(fvSolBackup, fvSol, copy_option::overwrite_if_exists);
  if (exists(fvSch)) copy_file(fvSchBackup, fvSch, copy_option::overwrite_if_exists);
  
}

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands
)
{
  redi::opstream proc;  
  std::vector<string> args;
  args.push_back("--use-offscreen-rendering");
  //ofc.forkCommand(proc, location, "pvpython", args);
  
  path tempfile=absolute(unique_path("%%%%%%%%%.py"));
  {
    std::ofstream tf(tempfile.c_str());
    tf << "from Insight.Paraview import *" << endl;
    BOOST_FOREACH(const std::string& cmd, pvpython_commands)
    {
      tf << cmd;
    }
    tf.close();
  }
  args.push_back(tempfile.c_str());
  ofc.executeCommand(location, "pvbatch", args);
  remove(tempfile);

}

}