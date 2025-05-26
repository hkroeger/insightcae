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

#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/translations.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/snappyhexmesh.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
TimeDirectoryList listTimeDirectories(
    const boost::filesystem::path& dir,
    const boost::filesystem::path& fta )
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
              auto td = itr->path();
              std::string fn=td.filename().string();
              if (isNumber(fn))
              {
                  double time = toNumber<double>(fn);
                  if (!fta.empty())
                  {
                    if (exists(td/fta))
                      list[time]=td/fta;
                  }
                  else
                    list[time]=td;
              }
            }
        }
    }
    return list;
}


std::string getLatestTimeDirectory(
    const boost::filesystem::path& dir )
{
  auto tds = listTimeDirectories(dir);
  if (tds.size()<1)
  {
    throw insight::Exception(
          _("No time directories present in case %s"),
          dir.string().c_str() );
  }
  return tds.rbegin()->second.string();
}


std::string getOpenFOAMComponentLabel(int i, int ncmpt)
{
    std::string cmptname;
    if (ncmpt==1)
    {
        cmptname="";
    }
    else if (ncmpt==3)
    {
        switch (i)
        {
        case 0:
            cmptname="x";
            break;
        case 1:
            cmptname="y";
            break;
        case 2:
            cmptname="z";
            break;
        }
    }
    else if (ncmpt==6)
    {
        switch (i)
        {
        case 0:
            cmptname="xx";
            break;
        case 1:
            cmptname="xy";
            break;
        case 2:
            cmptname="xz";
            break;
        case 3:
            cmptname="yy";
            break;
        case 4:
            cmptname="yz";
            break;
        case 5:
            cmptname="zz";
            break;
        }
    }
    return cmptname;
}


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

void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify, bool ignoremissing, bool include_zones)
{
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.string();
  ::system(cmd.c_str());
  
  std::vector<std::string> files=list_of<std::string>("boundary")("faces")("neighbour")("owner")("points");
  if (include_zones)
  {
    files.push_back("pointZones");
    files.push_back("faceZones");
    files.push_back("cellZones");
  }
  if (purify)
  {
    for (const std::string& fname: files)
    {
      path gzname(fname.c_str()); gzname=(gzname.string()+".gz");
      if (exists(source/gzname)) 
      {
        cout<<"Copying file "<<gzname<<endl;
        if (exists(target/gzname)) remove(target/gzname);
        copy_file(source/gzname, target/gzname);
      }
      else if (exists(source/fname))
      {
        cout<<"Copying file "<<fname<<endl;
        if (exists(target/fname)) remove(target/fname);
        copy_file(source/fname, target/fname);
      }
      else 
        if (!ignoremissing) throw insight::Exception("Essential mesh file "+fname+" not present in "+source.string());
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

void linkPolyMesh(
    const boost::filesystem::path& from,
    const boost::filesystem::path& to,
    const OFEnvironment* env)
{
  if (env)
  {
    boost::filesystem::path casedir=to.parent_path();
    std::cout<<"Creating case skeleton in "<<casedir<<endl;
    OpenFOAMCase cm(*env);
    cm.insert(new MeshingNumerics(cm));
    cm.createOnDisk(casedir);
  }
  
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.string();
  ::system(cmd.c_str());
  
  {
    std::string fname="boundary";

    // reset all patch types
    // (GGI patches require zones, which need to be recreated.
    // During zone creation, GGI types must not be defined...)
    // => read boundary file, reset types, write to target dir
    OFDictData::dict org_bnd;

    path gzname(fname.c_str()); gzname=(gzname.string()+".gz");
    if (exists(source/gzname)) 
    {
      cout<<"Processing file "<<gzname<<endl;
//      if (exists(target/gzname)) remove(target/gzname);
//      copy_file(source/gzname, target/gzname);
      std::ifstream compressedDict( (source/gzname).string() );
      boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
      in.push(boost::iostreams::gzip_decompressor());
      in.push(compressedDict);
      std::istream bf(&in);
      readOpenFOAMBoundaryDict(bf, org_bnd);
    }
    else if (exists(source/fname))
    {
      cout<<"Processing file "<<fname<<endl;
//      if (exists(target/fname)) remove(target/fname);
//      copy_file(source/fname, target/fname);
      std::ifstream bf( (source/fname).c_str() );
      readOpenFOAMBoundaryDict(bf, org_bnd);
    }
    else 
      throw insight::Exception("Essential mesh file "+fname+" not present in "+source.string());

    OFDictData::dictFile new_bnd;
    for (const auto& b: org_bnd)
    {
      const auto& pn = b.first;
      const auto& pd = b.second;

      OFDictData::dict nbd;
      nbd["type"]="patch";
      for (const auto& copy_key: std::vector<std::string>({"startFace", "nFaces"}))
      {
        nbd[copy_key]=boost::get<const int&>(
              boost::get<const OFDictData::dict&>(pd).at(copy_key)
              );
      }
      new_bnd[pn]=nbd;
    }

    {
      std::ofstream bf( (target/fname).c_str() );
      writeOpenFOAMBoundaryDict(bf, new_bnd);
    }

  }

  for (const std::string& fname:
		list_of<std::string>/*("boundary")*/("faces")("neighbour")("owner")("points")
		.convert_to_container<std::vector<std::string> >())
  {
    path gzname(fname.c_str()); gzname=(gzname.string()+".gz");
    if (exists(source/gzname)) create_symlink_force_overwrite(source/gzname, target/gzname);
    else if (exists(source/fname)) create_symlink_force_overwrite(source/fname, target/fname);
    else throw insight::Exception("Essential mesh file "+fname+" not present in "+source.string());
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

void mergeMeshes(const OpenFOAMCase& targetcase, const boost::filesystem::path& source, const boost::filesystem::path& target)
{
  if (targetcase.OFversion()<170)
  {
    boost::filesystem::path src=boost::filesystem::absolute(source);
    targetcase.executeCommand
    (
      target, "mergeMeshes", 
      {
       ".", ".",
       src.parent_path().string(), basename(src),
       "-noFunctionObjects"
      }
    );
  }
  else
    targetcase.executeCommand
    (
      target, "mergeMeshes", 
      {
       ".",
       boost::filesystem::absolute(source).string()
      }
    );
}


void mapFields
(
  const OpenFOAMCase& targetcase, 
  const boost::filesystem::path& source, 
  const boost::filesystem::path& target,
  bool parallelTarget,
  const std::vector<std::string>& fields
)
{
  std::string execname="mapFields";

  if (!boost::filesystem::exists(source/"system"/"controlDict"))
      throw insight::Exception("Source case for field mapping does not exist or does not contain a controlDict: "+source.string());

  if (!boost::filesystem::exists(target/"system"/"controlDict"))
      throw insight::Exception("Target case for field mapping does not exist or does not contain a controlDict: "+target.string());

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
    insight::Warning("A mapFieldsDict is existing. It will be used.");
  }

  std::vector<string> args =
    {
     boost::filesystem::absolute(source).string(),
     "-sourceTime", "latestTime"
    };

  if (parallelTarget) 
    args.push_back("-parallelTarget");
  
  
  if (targetcase.OFversion()>=230)
  {
    if (targetcase.requiredMapMethod()==OpenFOAMCase::directMapMethod)
    {
      args.push_back("-mapMethod");
      args.push_back("mapNearest");
//       execname="mapFields22";
//       args.push_back("-noFunctionObjects");
    }
  }

  if (targetcase.OFversion()>=230 && targetcase.OFversion()<300 && (fields.size()>0) && (execname!="mapFields22") )
  {
    std::ostringstream os;
    os<<"(";
    for (const std::string& fn: fields)
    {
      os<<" "<<fn;
    }
    os<<" )";
    
    args.push_back("-fields");
    args.push_back(os.str());
  }

//   if (targetcase.OFversion()>=220) execname="mapFields22";
  try
  {
    targetcase.executeCommand
    (
        target, execname, args
    );
  }
  catch (const std::exception& e)
  {
      if (targetcase.requiredMapMethod()==OpenFOAMCase::directMapMethod)
      {
          throw insight::Exception(std::string("mapFields failed! Error: ")+e.what());
      } else
      {
        // retry without interpolation
        args.push_back("-mapMethod");
        args.push_back("mapNearest");          
        try
        {
            targetcase.executeCommand
            (
                target, execname, args
            );
        }
        catch (const std::exception& e2)
        {
            throw insight::Exception(
                std::string("mapFields with interpolation failed. Retried with nearest cell matching and this attempt failed as well! Error: ")
                +e2.what());
        }
      }
  }
  
  // latest OF versions rename fields, which were not mapped. Rename them back...
  if (targetcase.OFversion()>=400)
  {
    directory_iterator end_itr; // default construction yields past-the-end
    for ( directory_iterator itr( target / "0" ); itr != end_itr; ++itr )
    {
        if ( is_regular_file(itr->status()) )
        {
            boost::filesystem::path fname = itr->path();
            std::cout<<fname<<std::endl;
            if (fname.extension().string()==".unmapped")
            {
                boost::filesystem::path orgname = fname;
                orgname.replace_extension("");
                std::cout<<"MOVE: "<<fname.string()<<" => "<<orgname.string()<<std::endl;
                rename(fname, orgname);
            }
        }
    }      
  }
}


void resetMeshToLatestTimestep(const OpenFOAMCase& /*c*/, const boost::filesystem::path& location, bool ignoremissing, bool include_zones, bool is_parallel)
{
    if (!is_parallel)
    {
        TimeDirectoryList times = listTimeDirectories(boost::filesystem::absolute(location));
        if (times.size()>0)
        {
            boost::filesystem::path lastTime = times.rbegin()->second;
            
            if (!ignoremissing) remove_all(location/"constant"/"polyMesh");
            copyPolyMesh(lastTime, location/"constant", true, ignoremissing, include_zones);
            
            for (const TimeDirectoryList::value_type& td: times)
            {
                remove_all(td.second);
            }
        }
    }
    else
    {
        directory_iterator end_itr; // default construction yields past-the-end
        for ( directory_iterator itr( location ); itr != end_itr; ++itr )
        {
            if ( is_directory(itr->status()) )
            {
                std::string dn=itr->path().filename().string();
                if ( starts_with(dn, "processor") )
                {
                    boost::filesystem::path curploc=itr->path();
                    
                    TimeDirectoryList times = listTimeDirectories(boost::filesystem::absolute(curploc));
                    if (times.size()>0)
                    {
                        boost::filesystem::path lastTime = times.rbegin()->second;
                        
                        if (!ignoremissing) remove_all(curploc/"constant"/"polyMesh");
                        copyPolyMesh(lastTime, curploc/"constant", true, ignoremissing, include_zones);
                        
                        for (const TimeDirectoryList::value_type& td: times)
                        {
                            remove_all(td.second);
                        }
                    }
                }
            }
        }
        
    }
}

void runPotentialFoam
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  int np
)
{
  path control(boost::filesystem::absolute(location)/"system"/"controlDict");
  path controlBackup(control); controlBackup.replace_extension(".potf");
  path fvSol(boost::filesystem::absolute(location)/"system"/"fvSolution");
  path fvSolBackup(fvSol); fvSolBackup.replace_extension(".potf");
  path fvSch(boost::filesystem::absolute(location)/"system"/"fvSchemes");
  path fvSchBackup(fvSch); fvSchBackup.replace_extension(".potf");
  path fvOpt(boost::filesystem::absolute(location)/"system"/"fvOptions");
  path fvOptBackup(fvOpt); fvOptBackup.replace_extension(".potf");
  
  if (exists(control)) copy_file(control, controlBackup, copy_option::overwrite_if_exists);
  if (exists(fvSol)) copy_file(fvSol, fvSolBackup, copy_option::overwrite_if_exists);
  if (exists(fvSch)) copy_file(fvSch, fvSchBackup, copy_option::overwrite_if_exists);
  if (exists(fvOpt)) 
  {
   copy_file(fvOpt, fvOptBackup, copy_option::overwrite_if_exists);
   remove(fvOpt);
  }
  
  OFDictData::dictFile controlDict;
  controlDict["deltaT"]=1.0;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=1.0;
  controlDict["writeControl"]="timeStep";
  controlDict["writeInterval"]=1;
  controlDict["purgeWrite"]=0;
  controlDict["writeFormat"]="ascii";
  controlDict["writePrecision"]=8;
  controlDict["writeCompression"]="compressed";
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
//  OFDictData::list l;
//  l.push_back("\"libextendedFixedValueBC.so\"");
//  controlDict.getList("libs")=l;
  //controlDict.subDict("functions");
  
  OFDictData::dictFile fvSolution;
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  
  std::string fieldName="p";
  if (cm.OFversion()>=300) fieldName="Phi";
  solvers[fieldName]=cm.stdSymmSolverSetup(1e-7, 0.01);
  
  fvSolution.subDict("relaxationFactors");
  std::string solkey="potentialFlow";
  if (cm.OFversion()<170) solkey="SIMPLE";
  OFDictData::dict& potentialFlow=fvSolution.subDict(solkey);
  potentialFlow["nNonOrthogonalCorrectors"]=3;
  
  OFDictData::dictFile fvSchemes;
  fvSchemes.subDict("ddtSchemes");
  fvSchemes.subDict("gradSchemes");
  fvSchemes.subDict("divSchemes");
  fvSchemes.subDict("laplacianSchemes");
  fvSchemes.subDict("interpolationSchemes");
  fvSchemes.subDict("snGradSchemes");
  fvSchemes.subDict("fluxRequired");
  
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

  OFDictData::dict mpd;
  mpd["default"]="areaAveraging";
  mpd["p"]="areaAveraging";
  mpd["U"]="areaAveraging";
  mpd["k"]="fluxAveraging";
  mpd["epsilon"]="fluxAveraging";
  mpd["omega"]="fluxAveraging";
  mpd["nuTilda"]="fluxAveraging";
  fvSchemes["mixingPlane"]=mpd;
  
  // then write to file
  {
    std::ofstream f(control.c_str());
    writeOpenFOAMDict(f, controlDict, boost::filesystem::basename(control));
    f.close();
  }
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
  cm.runSolver(location, analyzer, "potentialFoam", np,
			 list_of<std::string>("-noFunctionObjects"));

  if (exists(controlBackup)) copy_file(controlBackup, control, copy_option::overwrite_if_exists);
  if (exists(fvSolBackup)) copy_file(fvSolBackup, fvSol, copy_option::overwrite_if_exists);
  if (exists(fvSchBackup)) copy_file(fvSchBackup, fvSch, copy_option::overwrite_if_exists);
  if (exists(fvOptBackup)) copy_file(fvOptBackup, fvOpt, copy_option::overwrite_if_exists);
  
}

boost::mutex runPvPython_mtx;

void runPvPython
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
  const std::vector<std::string> pvpython_commands,
  bool keepScript
)
{
  boost::mutex::scoped_lock lock(runPvPython_mtx);
  
//  redi::opstream proc;
  std::vector<string> args;
  args.push_back("--force-offscreen-rendering");
  std::string machine=""; // execute always on local machine
  //ofc.forkCommand(proc, location, "pvpython", args);
  
  path tempfile=absolute(unique_path("%%%%%%%%%.py"));
  {
    std::ofstream tf(tempfile.c_str());
    tf << "from Insight.Paraview import *" << endl;
    for (const std::string& cmd: pvpython_commands)
    {
      tf << cmd;
    }
    tf.close();
  }
  args.push_back(tempfile.string());
  ofc.executeCommand(location, "pvbatch-offscreen", args, NULL, 0, &machine);

  if (!keepScript) remove(tempfile);

}



arma::mat matchValue(const std::string& vs)
{
  boost::regex re_vector ("^\\(([^ ]+) ([^ ]+) ([^ ]+)\\)$");
  try {
     arma::mat d;
     d << toNumber<double> ( vs ) ;
     return d;
  }
  catch (...)
  {
    boost::match_results<std::string::const_iterator> w;
    if (!boost::regex_match ( vs, w, re_vector))
      throw insight::Exception("reported value of patch integral was neither scalar nor vector!");
    arma::mat d;
    d << toNumber<double> ( w[1] )
      << toNumber<double> ( w[2] )
      << toNumber<double> ( w[3] );
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

patchArea::patchArea(const OpenFOAMCase& cm, const boost::filesystem::path& location,
                    const std::string& patchName)
{

  std::vector<std::string> output;
  cm.executeCommand ( location, "patchArea", list_of(patchName), &output );

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




arma::mat readTextFile(std::istream& f)
{
  CurrentExceptionContext ex("reading tabular data from input stream");

  arma::mat data;
  std::vector< std::vector<double> > fd;

  std::string line;
  int iline=0;
  while (getline(f, line))
  {
    iline++;
    CurrentExceptionContext ex(3, str(format("reading line %d (containing \"%s\")")%iline%line), false);

    algorithm::trim_left(line);
    char fc; istringstream(line) >> fc; // get first char
    if ( (line.size()==0) || (fc=='#') )
    {
      // comment
    }
    else
    {
      erase_all ( line, "(" );
      erase_all ( line, ")" );
      replace_all ( line, ",", " " );
      replace_all ( line, "\t", " " );
      while (line.find("  ")!=std::string::npos)
      {
        replace_all ( line, "  ", " " );
      }

      std::vector<std::string> strs;
      boost::split(strs, line, is_any_of(" "));

//      for (const auto& s: strs) std::cout<<s<<" >> "; std::cout<<std::endl;

      std::vector<double> vals;
      transform(strs.begin(), strs.end(), std::back_inserter(vals),
                [](const std::string& s) { return insight::toNumber<double>(s); });

      fd.push_back(vals);
    }
  }

  if (fd.size()==0)
  {
    data=arma::mat();
  }
  else
  {
    data.reshape(fd.size(), fd[0].size());
    size_t ir=0;
    for (const auto& r: fd)
    {
      if (r.size()!=data.n_cols)
        throw insight::Exception(str(format("Wrong number of cols (%d) in data row %d. Expected %d.")
                                     %r.size()%ir%data.n_cols));
      else
      {
        for (size_t j=0;j<r.size(); j++)
          data(ir,j)=r[j];
      }
      ir++;
    }
  }

  return data;
}



arma::mat readParaviewCSV(const boost::filesystem::path& file, std::map<std::string, int>* headers)
{
//   boost::filesystem::path file=filetemplate.parent_path() 
//     / (filetemplate.filename().stem().string() + lexical_cast<string>(num) + filetemplate.filename().extension().string());

  cout << "Reading "<<file<<endl;
    
  std::ifstream f(file.c_str());
  
  std::vector<double> data;
  
  std::string headerline;
  getline(f, headerline);
  std::vector<std::string> colnames;
  boost::split(colnames, headerline, boost::is_any_of(","));
  for(size_t i=0; i<colnames.size(); i++)
  {
    (*headers)[colnames[i]]=i;
  }
  
  while(!f.eof())
  {
    std::string line;
    getline(f, line);
    if (f.fail()) break;
    
    std::vector<std::string> cols;
    boost::split(cols, line, boost::is_any_of(","));
    for(size_t i=0; i<cols.size(); i++)
    {
      data.push_back(toNumber<double>(cols[i]));
    }
  }
  
  return arma::mat(data.data(), colnames.size(), data.size()/colnames.size()).t();
}

typedef std::map<std::string, int> ColumnDescription;

bool equal_columns(const ColumnDescription& c1, const ColumnDescription& c2)
{
  bool ok = (c1.size()==c2.size());
  for (const ColumnDescription::value_type& c1e: c1)
  {
//     cout<<"col="<<c1e.first<<" ("<<c1e.second<<") >>> ";
    ColumnDescription::const_iterator i2=c2.find(c1e.first);
    if (i2 == c2.end()) { ok=false; /*cout<<"not found"<<endl;*/ }
    else { ok = ok && (c1e.second == i2->second); /*cout<<i2->second<<endl;*/ }
  }
  return ok;
}

std::vector<arma::mat> readParaviewCSVs(const boost::filesystem::path& filetemplate, ColumnDescription* headers)
{
//   if (num<0)
//     throw insight::Exception("readParaviewCSV: Reading and combining all files is not yet supported!");
//   
//   boost::filesystem::path file=filetemplate.parent_path() 
//     / (filetemplate.filename().stem().string() + lexical_cast<string>(num) + filetemplate.filename().extension().string());
// 
  
//   ColumnDescription header;
//   std::vector<arma::mat> result;
  
  typedef std::map<std::string, std::vector< arma::mat> > AllData;
  AllData alldata;
  
  boost::regex fname_pattern(filetemplate.filename().stem().string() + "[0-9]+" + filetemplate.filename().extension().string());
  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( filetemplate.parent_path() );
	itr != end_itr; ++itr )
  {
    if ( is_regular_file(itr->status()) )
    {
//       cout<<"file: "<<itr->path().filename().string()<<endl;
      if ( boost::regex_match( itr->path().filename().string(), fname_pattern ) )
      {
// 	cout<<"OK"<<endl;
	std::map<std::string, int> thisheaders;
	arma::mat r = readParaviewCSV(itr->path().c_str(), &thisheaders);
// 	cout<< (r.n_rows) <<" & "<<thisheaders.size()<<endl;
	if ( (thisheaders.size()>0) && (r.n_rows>0))
	{
	  if (alldata.size()==0)
	  {
	    // insert all cols
	    for (const ColumnDescription::value_type& cd: thisheaders)
	    {
	      alldata[cd.first].push_back(r.col(cd.second));
	    }
	  }
	  else
	  {
	    std::set<std::string> vc;
	    for (const AllData::value_type& adt: alldata) vc.insert(adt.first);
	    for (const ColumnDescription::value_type& cdt: thisheaders)
	    {
	      AllData::iterator j=alldata.find(cdt.first); // try to find each column of this CSV in alldata
	      if (j!=alldata.end()) // if present, append
	      {
// 		cout<<"append "<<cdt.first<<endl;
		j->second.push_back(r.col(cdt.second));
		vc.erase(vc.find(cdt.first));
	      }
	    }
	    // remove all cols, that are not present
	    for (const std::string& vci: vc)
	    {
// 	      cout<<"Remove "<<vci<<endl;
	      alldata.erase(alldata.find(vci));
	    }
	  }
// 	  if (result.size()==0)
// 	  {
// 	    header=thisheaders;
// 	  }
// 	  else
// 	  {
// 	    if (!equal_columns(header, thisheaders))
// 	    {
// 	      throw insight::Exception("incompatible file columns!");
// 	    }
// 	  }
// 	  result.push_back(r);
	}
      } /*else { cout<<"NO"<<endl; }*/
    }
  }

//   if (headers) *headers=header;
//   return result;

  int np=0;
  if (alldata.size()>0) np=alldata.begin()->second.size();

  std::vector<arma::mat> result(np);
  
  if (headers)
  {
    int j=0;
    for (const AllData::value_type& cv: alldata)
    {
      (*headers)[cv.first]=j++;
    }
  }
  
  for (const AllData::value_type& cv: alldata)
  {
    const std::vector< arma::mat>& profs=cv.second;
    int k=0;
    for (const arma::mat& col: profs)
    {
      arma::mat& cumprof=result[k++];
      if (cumprof.n_cols==0) 
	cumprof=col;
      else
	cumprof=join_rows(cumprof, col); // append column
    }
  }
  
  return result;
}

std::string readSolverName(const boost::filesystem::path& ofcloc)
{
  OFDictData::dict controlDict;
  std::ifstream cdf( (ofcloc/"system"/"controlDict").c_str() );
  readOpenFOAMDict(cdf, controlDict);
  return controlDict.getString("application");
}


int readDecomposeParDict(const boost::filesystem::path& ofcloc)
{
  OFDictData::dict decomposeParDict;
  readOpenFOAMDict(ofcloc/"system"/"decomposeParDict", decomposeParDict);
  //cout<<decomposeParDict<<endl;
  return decomposeParDict.getInt("numberOfSubdomains");
}

std::string readTurbulenceModelName(const OpenFOAMCase& c, const boost::filesystem::path& ofcloc)
{
  OFDictData::dict RASPropertiesDict;
  std::ifstream cdf( (ofcloc/"constant"/"RASProperties").c_str() );
  readOpenFOAMDict(cdf, RASPropertiesDict);
  //cout<<decomposeParDict<<endl;
  if (c.OFversion()<300)
  {
    return RASPropertiesDict.getString("RASModel");
  }
  else
  {
    return RASPropertiesDict.subDict("RAS").getString("RASModel");
  }
}



MeshQualityInfo::MeshQualityInfo()
{
  ncells=-1;
  nhex=-1;
  nprism=-1;
  ntet=-1;
  npoly=-1;
  nmeshregions=-1;
  bb_min=vec3(-DBL_MAX, -DBL_MAX, -DBL_MAX);
  bb_max=vec3(DBL_MAX, DBL_MAX, DBL_MAX);
  max_aspect_ratio=-1;
  min_faceA="";
  min_cellV="";
  max_nonorth=-1;
  avg_nonorth=-1;
  max_skewness=-1;
  n_severe_nonorth=0;
  n_neg_facepyr=0;
  n_severe_skew=0;
}


std::vector<MeshQualityInfo> getMeshQuality(const OpenFOAMCase& cm, const boost::filesystem::path& location,
                       const std::vector<string>& addopts
                      )
{
  std::vector<std::string> opts;
  copy(addopts.begin(), addopts.end(), back_inserter(opts));

  std::vector<std::string> output;
  cm.executeCommand(location, "checkMesh", opts, &output);

  // Pattern
  enum Section {MeshStats, CellTypes, Topology, Geometry} ;
  boost::regex SectionIntroPattern[] = {
    boost::regex("^Mesh stats$"),
    boost::regex("^Overall number of cells of each type:$"),
    boost::regex("^Checking topology...$"),
    boost::regex("^Checking geometry...$")
  };
  Section curSection;

  boost::regex re_time("^ *Time = (.+)$");
  boost::match_results<std::string::const_iterator> what;
  std::string time="";



  typedef std::vector<MeshQualityInfo> MQInfoList;
  MQInfoList mqinfos;
  MeshQualityInfo curmq;
  for (const std::string& line: output)
  {
    if (boost::regex_match(line, what, re_time))
    {
      if (curmq.time!="")
      {
        mqinfos.push_back(curmq);
      }
      curmq.time=what[1];
    }
    for (int i=0; i<4; i++)
      if (boost::regex_match(line, what, SectionIntroPattern[i])) curSection=static_cast<Section>(i);

//     try{
    switch (curSection)
    {
      case MeshStats:
      {
        if (boost::regex_match(line, what, boost::regex("^ *cells: *([0-9]+)$")))
          curmq.ncells=lexical_cast<int>(what[1]);
        break;
      }
      case CellTypes:
      {
        if (boost::regex_match(line, what, boost::regex("^ *hexahedra: *([0-9]+)$")))
          curmq.nhex=lexical_cast<int>(what[1]);
        if (boost::regex_match(line, what, boost::regex("^ *prisms: *([0-9]+)$")))
          curmq.nprism=lexical_cast<int>(what[1]);
        if (boost::regex_match(line, what, boost::regex("^ *tetrahedra: *([0-9]+)$")))
          curmq.ntet=lexical_cast<int>(what[1]);
        if (boost::regex_match(line, what, boost::regex("^ *polyhedra: *([0-9]+)$")))
          curmq.npoly=lexical_cast<int>(what[1]);
        break;
      }
      case Topology:
      {
        if (boost::regex_match(line, what, boost::regex("^ *Number of regions: *([^ ]+) .*$")))
          curmq.nmeshregions=lexical_cast<int>(what[1]);
        break;
      }
      case Geometry:
      {
        if (boost::regex_match(line, what, boost::regex("^ *Overall domain bounding box \\(([^ ]+) ([^ ]+) ([^ ]+)\\) \\(([^ ]+) ([^ ]+) ([^ ]+)\\)$")))
        {
          curmq.bb_min=vec3( toNumber<double>(what[1]), toNumber<double>(what[2]), toNumber<double>(what[3]) );
          curmq.bb_max=vec3( toNumber<double>(what[4]), toNumber<double>(what[5]), toNumber<double>(what[6]) );
        }
        if (boost::regex_match(line, what, boost::regex("^ *Max aspect ratio = ([^ ]+) .*$")))
        {
          curmq.max_aspect_ratio=toNumber<double>(what[1]);
        }
        if (boost::regex_match(line, what, boost::regex("^ *Minimum face area = *([^ ]+)\\. Maximum face area = *([^ ]+)\\..*$")))
        {
          cout<<what[1]<<endl;
          curmq.min_faceA=std::string(what[1]); // is a very small value, keep as string
          cout<<curmq.min_faceA<<endl;
          //sscanf(string(what[1]).data(), "%g", &curmq.min_faceA);
        }
        if (boost::regex_match(line, what, boost::regex("^ *Min volume = *([^ ]+)\\. Max volume.*$")))
        {
          cout<<what[1]<<endl;
          curmq.min_cellV=std::string(what[1]); // is a very small value, keep as string
          cout<<curmq.min_cellV<<endl;
          //sscanf(string(what[1]).data(), "%g", &curmq.min_cellV);
        }
        if (boost::regex_match(line, what, boost::regex("^ *Mesh non-orthogonality Max: ([^ ]+) average: ([^ ]+)$")))
        {
          curmq.max_nonorth=toNumber<double>(what[1]);
          curmq.avg_nonorth=toNumber<double>(what[2]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Number of severely non-orthogonal \\(> ([^ ]+) degrees\\) faces: ([^ ]+)\\..*$")))
        {
          curmq.n_severe_nonorth=toNumber<double>(what[2]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Max skewness = ([^ ]+), ([^ ]+) highly skew faces.*$")))
        {
          curmq.n_severe_skew=toNumber<double>(what[2]);
          curmq.max_skewness=toNumber<double>(what[1]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Max skewness = ([^ ]+) OK.*$")))
        {
          curmq.n_severe_nonorth=toNumber<double>(what[1]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Error in face pyramids: ([^ ]+) faces are incorrectly oriented.*$")))
        {
          curmq.n_neg_facepyr=toNumber<double>(what[1]);
        }
        break;
      }
    }
//     }
//     catch(boost::bad_lexical_cast& e) {
//      cout<<"Lexical_cast: "<<e.what()<<endl;
//     }
  }
  if (curmq.time!="") mqinfos.push_back(curmq);

  return mqinfos;
}
  
void meshQualityReport(const OpenFOAMCase& cm, const boost::filesystem::path& location, 
		       ResultSetPtr results,
		       const std::vector<string>& addopts
		      )
{
  auto mqinfos=getMeshQuality(cm, location, addopts);
  
  for (const MeshQualityInfo& mq: mqinfos)
  {
    results->insert
    (
     "Mesh quality at time "+mq.time,
     std::make_unique<AttributeTableResult>
     (
        AttributeTableResult::AttributeNames{
          {"Number of cells"},
          {"thereof hexahedra"},
          {"prisms"},
          {"tetrahedra"},
          {"polyhedra"},

          {"Number of mesh regions"},

          {"Domain extent (X)"},
          {"Domain extent (Y)"},
          {"Domain extent (Z)"},

          {"Max. aspect ratio"},
          {"Min. face area"},
          {"Min. cell volume"},

          {"Max. non-orthogonality"},
          {"Avg. non-orthogonality"},
          {"Max. skewness"},

          {"No. of severely non-orthogonal faces"},
          {"No. of negative face pyramids"},
          {"No. of severely skew faces"}
        },

        AttributeTableResult::AttributeValues{
          mq.ncells, mq.nhex, mq.nprism, mq.ntet, mq.npoly,
          mq.nmeshregions,
          mq.bb_max(0)-mq.bb_min(0), mq.bb_max(1)-mq.bb_min(1), mq.bb_max(2)-mq.bb_min(2),
          mq.max_aspect_ratio, mq.min_faceA, mq.min_cellV,
          mq.max_nonorth, mq.avg_nonorth, mq.max_skewness,
          mq.n_severe_nonorth, mq.n_neg_facepyr, mq.n_severe_skew
        },

        "Mesh Quality", "", ""
     )
    ).setOrder(0);
  }
}



void currentNumericalSettingsReport
(
  const OpenFOAMCase& /*cm*/,
  const boost::filesystem::path& location, 
  ResultSetPtr results
)
{
  double order=990;
  for
  (
    const boost::filesystem::path& dictname:
    list_of<boost::filesystem::path> 
      ("system/controlDict")("system/fvSchemes")("system/fvSchemes")
      ("constant/RASProperties")("constant/LESProperties")
      ("constant/transportProperties")
      .convert_to_container<std::vector<boost::filesystem::path> >()
  )
  {
    try
    {
      OFDictData::dict cdict;
      std::ifstream cdf( (location/dictname).c_str() );
      readOpenFOAMDict(cdf, cdict);
//       cout<<cdict<<endl;
      
      std::ostringstream latexCode;
      latexCode<<"\\begin{verbatim}\n"
	<<cdict
	<<"\n\\end{verbatim}\n";
      
      std::string elemname=dictname.string();
      replace_all(elemname, "/", "_");
      results->insert("dictionary_"+elemname,
        std::unique_ptr<Comment>(new Comment
	(
	latexCode.str(), 
	"Contents of "+dictname.string()
      ))).setOrder(order);    
      order+=1.;
    }
    catch (...)
    {
      cout<<"File "<<dictname.string()<<" not readable."<<endl;
      // Ignore errors, files may not exists in current setup
    }
  }
}

/**
 * read profile of viscous force along wall
 * return (x, fx_mean, fy_mean, fz_mean, fx, fy, fz)
 */
arma::mat viscousForceProfile
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& axis, int n,
  const std::vector<std::string>& addopts
)
{
  std::vector<std::string> opts;
  opts.push_back(OFDictData::toString(OFDictData::vector3(axis)));
  opts.push_back("(viscousForce viscousForceMean)");
  opts.push_back("-walls");
  opts.push_back("-n");
  opts.push_back(lexical_cast<string>(n));
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  
  std::vector<std::string> output;
  cm.executeCommand(location, "binningProfile", opts, &output);
  
  path pref=location/"postProcessing"/"binningProfile";
  TimeDirectoryList tdl=listTimeDirectories(pref);
  path lastTimeDir=tdl.rbegin()->second;
  arma::mat vfm;
  vfm.load( ( lastTimeDir/"walls_viscousForceMean.dat").string(), arma::raw_ascii);
  arma::mat vf;
  vf.load( (lastTimeDir/"walls_viscousForce.dat").string(), arma::raw_ascii);
  
  return arma::mat(join_rows(vfm, vf.cols(1, vf.n_cols-1)));
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

void surfaceFeatureExtract
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string& surfaceName
)
{
  OFDictData::dictFile sfeDict;
  
  OFDictData::dict opts;
  
  opts["extractionMethod"]="extractFromSurface";
  
  OFDictData::dict coeffs;
  coeffs["includedAngle"]=120.0;
  coeffs["geometricTestOnly"]=true;
  opts["extractFromSurfaceCoeffs"]=coeffs;
  
  opts["writeObj"]=false;

  sfeDict[surfaceName]=opts;
  
  // then write to file
  sfeDict.write( location / "system" / "surfaceFeatureExtractDict" );

  std::vector<std::string> opt;
//   opts.push_back("-latestTime");
  //if (overwrite) opts.push_back("-overwrite");
    
  cm.executeCommand(location, "surfaceFeatureExtract", opt);
}

void extrude2DMesh
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location, 
  const std::string& sourcePatchName,
  std::string sourcePatchName2,
  bool wedgeInsteadOfPrism,
  double distance,
  const arma::mat& offsetTranslation,
  const arma::mat& fixedDirection
)
{  
  
  if (sourcePatchName2=="") sourcePatchName2=sourcePatchName;
  OFDictData::dictFile extrDict;
  
  extrDict["constructFrom"]="patch";
  extrDict["sourceCase"]="\""+absolute(location).string()+"\"";
  extrDict["sourcePatches"]="("+sourcePatchName+")"; // dirty
  extrDict["exposedPatchName"]=sourcePatchName2;
  extrDict["flipNormals"]=false;
  extrDict["nLayers"]=1;
  extrDict["expansionRatio"]=1.0;

  if (wedgeInsteadOfPrism)
  {
    extrDict["extrudeModel"]="wedge";
    OFDictData::dict wc;
    wc["axisPt"]=OFDictData::vector3(vec3(0,0,0));
    wc["axis"]=OFDictData::vector3(vec3(-1,0,0));
    wc["angle"]=5.0;
    if (cm.OFversion()>=600)
        extrDict["sectorCoeffs"]=wc;
    else
        extrDict["wedgeCoeffs"]=wc;
  }
  else
  {
    OFDictData::dict lnc;
    lnc["thickness"]=distance;
    if (fixedDirection.n_elem==3)
      {
        extrDict["extrudeModel"]="linearDirection";
        lnc["direction"]=OFDictData::vector3(fixedDirection);
        extrDict["linearDirectionCoeffs"]=lnc;
      }
    else
      {
        extrDict["extrudeModel"]="linearNormal";
        extrDict["linearNormalCoeffs"]=lnc;
      }
  }


  extrDict["mergeFaces"]=false;
  extrDict["mergeTol"]=1e-8; // needs to be small for not collapsing prism layer faces
  
  boost::filesystem::path fname;
  if (cm.OFversion()<170)
    fname=boost::filesystem::path("constant")/"extrudeProperties";
  else
    fname=boost::filesystem::path("system") / "extrudeMeshDict";
  
  extrDict.write( location / fname );

  std::vector<std::string> opt;
  cm.executeCommand(location, "extrudeMesh", opt);

  if (!wedgeInsteadOfPrism)
  {
    opt.clear();
    opt={
      "-translate",
      OFDictData::toString(OFDictData::vector3(offsetTranslation))
    };
    cm.executeCommand(location, "transformPoints", opt);
  }
  else
  {
    opt.clear();
    opt={
       OFDictData::toString(OFDictData::vector3(0,0,0)),
       sourcePatchName,
       sourcePatchName2
    };
    cm.executeCommand(location, "flattenWedges", opt);
  }
}

void rotateMesh
(
  const OpenFOAMCase& cm, 
  const path& location, 
  const string& sourcePatchName, 
  int nc,
  const arma::mat& axis, 
  const arma::mat& p0  
)
{  
  
  OFDictData::dictFile extrDict;
  
  extrDict["constructFrom"]="patch";
  extrDict["sourceCase"]="\""+absolute(location).string()+"\"";
  if (cm.OFversion()>=230)
    extrDict["sourcePatches"]="("+sourcePatchName+")"; // dirty
  else
    extrDict["sourcePatch"]=sourcePatchName;
  extrDict["exposedPatchName"]=sourcePatchName;
  extrDict["flipNormals"]=false;
  extrDict["nLayers"]=nc;
  extrDict["expansionRatio"]=1.0;

  OFDictData::dict wc;
  wc["axisPt"]=OFDictData::vector3(p0);
  wc["axis"]=OFDictData::vector3(axis);
  wc["angle"]=360.0;
  if (cm.OFversion()<400)
  {
    extrDict["extrudeModel"]="wedge";
    extrDict["wedgeCoeffs"]=wc;
  } 
  else
  {
    extrDict["extrudeModel"]="sector";
    extrDict["sectorCoeffs"]=wc;
  }


  extrDict["mergeFaces"]=true;
  extrDict["mergeTol"]=1e-8; // needs to be small for not collapsing prism layer faces
  
  boost::filesystem::path fname;
  if (cm.OFversion()<170)
    fname=boost::filesystem::path("constant")/"extrudeProperties";
  else
    fname=boost::filesystem::path("system") / "extrudeMeshDict";
  
  extrDict.write( location / fname );

  std::vector<std::string> opt;
  cm.executeCommand(location, "extrudeMesh", opt);

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
      mir<<t<<toNumber<double>(what[3])<<toNumber<double>(what[4])<<toNumber<double>(what[5])<<endr;
      mar<<t<<toNumber<double>(what[6])<<toNumber<double>(what[7])<<toNumber<double>(what[8])<<endr;
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

arma::mat interiorPressureFluctuationProfile
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const arma::mat& axis, int n,
  const std::vector<std::string>& addopts
)
{
  std::vector<std::string> opts;
  opts.push_back(OFDictData::toString(OFDictData::vector3(axis)));
  opts.push_back("(pPrime2Mean)");
  opts.push_back("-interior");
  opts.push_back("-n");
  opts.push_back(lexical_cast<string>(n));
  copy(addopts.begin(), addopts.end(), back_inserter(opts));
  std::vector<std::string> output;
  cm.executeCommand(location, "binningProfile", opts, &output);
  
  path pref=location/"postProcessing"/"binningProfile";
  TimeDirectoryList tdl=listTimeDirectories(pref);
  path lastTimeDir=tdl.rbegin()->second;
  arma::mat vfm;
  vfm.load( ( lastTimeDir/"interior_pPrime2Mean.dat").string(), arma::raw_ascii);
  
  return vfm;
}


std::set<string>
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


void createPrismLayers
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& casePath,
  double finalLayerThickness, 
  bool relativeSizes, 
  const insight::PatchLayers& nLayers,
  double expRatio,
  bool twodForExtrusion,
  bool isalreadydecomposed,
  bool keepdecomposedafterfinish,
  ProgressDisplayer* progress
)
{
    
//   boost::ptr_vector<snappyHexMeshFeats::Feature> shm_feats;
    snappyHexMeshConfiguration::Parameters shm_cfg;
    
    shm_cfg.set_erlayer(expRatio)
      .set_tlayer(finalLayerThickness)
      .set_relativeSizes(relativeSizes)
      
      .set_doCastellatedMesh(false)
      .set_doSnap(false)
      .set_doAddLayers(true)
  ;
  for (const PatchLayers::value_type& pl: nLayers)
  {
//     shm_feats.push_back(new snappyHexMeshFeats::PatchLayers(snappyHexMeshFeats::PatchLayers::Parameters()
      shm_cfg.features.push_back( snappyHexMeshFeats::FeaturePtr( new snappyHexMeshFeats::PatchLayers( snappyHexMeshFeats::PatchLayers::Parameters()
      .set_nLayers(pl.second)
      .set_name(pl.first)
    )));
  }
  
  if (twodForExtrusion)
  {
//     setNoQualityCtrls(*qualityCtrls);
      shm_cfg.qualityCtrls = snappyHexMeshConfiguration::Parameters::disabled;
  }
  else
  {
      shm_cfg.qualityCtrls = snappyHexMeshConfiguration::Parameters::relaxed;
//      shm_cfg.qualityCtrls = snappyHexMeshConfiguration::Parameters::standard;
//     setRelaxedQualityCtrls(*qualityCtrls);
//     (*qualityCtrls)["maxConcave"]=180.0; //85.0;  
//     (*qualityCtrls)["minTetQuality"]=-1; //1e-40;  
  }
  
  shm_cfg.PiM.push_back(vec3(0,0,0));
  snappyHexMesh
  (
    cm, casePath,
//     OFDictData::vector3(vec3(0,0,0)),
//     shm_feats,
//     snappyHexMeshOpts::Parameters()
// //       .set_stopOnBadPrismLayer(p.getBool("mesh/prismfailcheck"))
//       .set_erlayer(expRatio)
//       .set_tlayer(finalLayerThickness)
//       .set_relativeSizes(relativeSizes)
//       
//       .set_doCastellatedMesh(false)
//       .set_doSnap(false)
//       .set_doAddLayers(true)
//       
//       .set_qualityCtrls(qualityCtrls)
// /*
//       .set_tlayer(p.getDouble("mesh/mlayer"))
//       .set_relativeSizes(true)
//   */
//     ,
    shm_cfg,
    true,
    isalreadydecomposed,
    keepdecomposedafterfinish,
    progress
  ); 
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
  opts.push_back(lexical_cast<std::string>(npts));
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

 
int find_files( const boost::filesystem::path & dir_path,         // in this directory,
                const std::string & file_name, // search for this name,
                std::vector<boost::filesystem::path> & path_found )            // placing path here if found
{
  int num=0;
  
  if ( !boost::filesystem::exists( dir_path ) ) return num;
  
  boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( dir_path );
        itr != end_itr;
        ++itr )
  {
    if ( boost::filesystem::is_directory(itr->status()) )
    {
      num+=find_files( itr->path(), file_name, path_found );
    }
    else if ( boost::filesystem::is_regular_file ( itr->status() ) )
    {
      if ( itr->path().filename() == file_name ) 
      {
	path_found.push_back(itr->path());
	num+=1;
      }
    }
  }
  return num;
}

std::vector<boost::filesystem::path> searchOFCasesBelow(const boost::filesystem::path& basepath)
{
  std::vector<boost::filesystem::path> cds, cases;
  find_files(basepath, "controlDict", cds);
  for (const boost::filesystem::path& cd: cds)
  {
    if ( boost::filesystem::basename(cd.parent_path()) == "system" )
    {
      cases.push_back( cd.parent_path().parent_path() );
    }
  }
  
  return cases;
}





defineType(HomogeneousAveragedProfile);
Analysis::Add<HomogeneousAveragedProfile> addHomogeneousAveragedProfile;


HomogeneousAveragedProfile::HomogeneousAveragedProfile(
    const std::shared_ptr<supplementedInputDataBase>& sp )
: AnalysisWithParameters(sp)
{}


ResultSetPtr HomogeneousAveragedProfile::operator()(ProgressDisplayer& /*displayer*/)
{
  OpenFOAMCase cm(OFEs::get(p().OFEname));
  
  arma::mat xs;
  
  if ( Parameters::grading_type::none == p().grading )
  {
    xs=linspace(0., 1., p().np);
  }
  else if ( Parameters::grading_type::towardsEnd == p().grading )
  {
    xs=cos(0.5*M_PI*(linspace(0., 1., p().np)-1.0));
  }
  else if ( Parameters::grading_type::towardsStart == p().grading )
  {
    xs=1.0 - cos(0.5*M_PI*linspace(0., 1., p().np));
  }

  
  arma::mat pts = arma::trans(
      p().L * arma::trans(xs)
     +
      p().p0 * arma::ones(1,p().np)
  );
  
  boost::ptr_vector<sampleOps::set> sets;
  sets.push_back(new sampleOps::linearAveragedPolyLine(sampleOps::linearAveragedPolyLine::Parameters()
    .set_points( pts )
    .set_dir1(p().homdir1)
    .set_dir2(p().homdir2)
    .set_nd1(p().n_homavg1)
    .set_nd2(p().n_homavg2)
    .set_name(p().profile_name)
  ));

  auto casepath = p().casepath->filePath(executionPath());

  sample(
      cm, casepath,
      std::container_type_cast<std::vector<std::string> >(
          p().fields ),
      sets);
      
  sampleOps::ColumnDescription cd;
  arma::mat data = dynamic_cast<sampleOps::linearAveragedPolyLine*>(&sets[0])
    -> readSamples(cm, casepath, &cd);

  auto results = createResultSet();
  Ordering so;
  
  for (const std::string& fieldname: p().fields)
  {
    int c=cd[fieldname].col;
    int ncmpt=cd[fieldname].ncmpt;
    
    PlotCurveList crvs;
    for (int i=0; i<ncmpt; i++)
    {
      
      std::string cmptname = getOpenFOAMComponentLabel(i, ncmpt);
      
      std::string lxcmptname="";
      if (cmptname!="") lxcmptname="_{"+cmptname+"}";
      
      crvs.push_back(PlotCurve(data.col(0), data.col(c+i), fieldname+cmptname, "w l t '$"+fieldname+lxcmptname+"$'"));
    }
    
    addPlot
    (
      results, casepath, "profiles_"+fieldname,
      "$x / m$", fieldname,
      crvs,
      "Profiles of field "+fieldname
    ) 
    .setOrder(so.next());
    
  }
  
  return results;
}

// addToAnalysisFactoryTable(HomogeneousAveragedProfile);


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

void calcR
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::vector<std::string>& addopts
)
{
    if (cm.OFversion()<400)
    {
        cm.executeCommand( location, "R", addopts );
    }
    else
    {
        std::string solver = readSolverName(location);
        
        std::vector<std::string> opts = addopts;
        opts.insert(opts.begin(), "R");
        opts.insert(opts.begin(), "-func");
        opts.insert(opts.begin(), "-postProcess");
        cm.executeCommand( location, solver, opts );
    }
}

void calcLambda2
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::vector<std::string>& addopts
)
{
    if (cm.OFversion()<400)
    {
        cm.executeCommand( location, "Lambda2", addopts );
    }
    else
    {
        std::vector<std::string> opts = addopts;
        opts.insert(opts.begin(), "Lambda2");
        opts.insert(opts.begin(), "-func");
        cm.executeCommand( location, "postProcess", opts );
    }
}


bool checkIfAnyFileIsNewerOrNonexistent
(
    boost::filesystem::path orig,
    boost::filesystem::path copy,
    bool recursive
)
{
    using namespace boost::filesystem;
    
    bool anynewerornonexistent=false;
    directory_iterator end_itr; // default construction yields past-the-end
    for 
    ( 
        directory_iterator itr( orig );
        itr != end_itr;
        ++itr 
    )
    {
        boost::filesystem::path curname=itr->path().filename();
        
        if ( is_directory(itr->status()) )
        {
            if (recursive)
            {
                anynewerornonexistent |= checkIfAnyFileIsNewerOrNonexistent(orig/curname, copy/curname);
            }
        }
        else
        {
            if (!exists(copy/curname))
            {
                std::cout<<"NOT EXISTING IN "<<copy<<": "<<curname<<std::endl;
                anynewerornonexistent = true;
            }
            else if ( last_write_time(orig/curname) > last_write_time(copy/curname) )
            {
                std::cout<<"NEWER IN "<<orig<<": "<<curname<<std::endl;
                anynewerornonexistent = true;
            }
        }
    }
    
    return anynewerornonexistent;
}


ParallelTimeDirectories::ParallelTimeDirectories(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location
    )
    : cm_(cm), location_(location)
{
    serTimes_ = listTimeDirectories(location);

    directory_iterator end_itr; // default construction yields past-the-end
    const boost::regex filter( "processor[0-9]+" );
    for ( directory_iterator i( location ); i != end_itr; i++ )
    {
        if ( is_directory(i->status()) )
        {
            std::string fn=i->path().filename().string();
            boost::smatch what;
            if ( boost::regex_match( i->path().filename().string(), what, filter ) )
            {
                procDirs_.insert(i->path());
            }
        }
    }

    auto proc0 = location/"processor0";
    if (boost::filesystem::exists(proc0)
        && boost::filesystem::is_directory(proc0))
    {
        proc0Times_=listTimeDirectories(proc0);
    }
}




bool ParallelTimeDirectories::proc0TimeDirNeedsReconst(
    const boost::filesystem::path& ptdpath ) const
{
    // search for identical time directory *name*
    auto iser=std::find_if(
        serTimes_.begin(), serTimes_.end(),
        [&](const TimeDirectoryList::value_type& td)
        {
            return boost::filesystem::basename(ptdpath)
                == boost::filesystem::basename(td.second);
        }
    );

    if (iser==serTimes_.end())
    {
        // no matching serial time dir present
        return true;
    }
    else
    {
        // its there, but up-to-date?
        return checkIfAnyFileIsNewerOrNonexistent(ptdpath, iser->second, true);
    }

    return false;
}




std::set<boost::filesystem::path> ParallelTimeDirectories::newParallelTimes(bool filterOutInconsistent) const
{
    std::set<boost::filesystem::path> result;

    for (const auto& ptd: proc0Times_)
    {
        if (proc0TimeDirNeedsReconst(ptd.second))
        {
            if ( !filterOutInconsistent
                 ||
                 (filterOutInconsistent && !isParallelTimeDirInconsistent(ptd.second.filename())) )
            {
                // no matching serial time dir present
                result.insert(ptd.second.filename());
            }
        }
    }

    return result;
}




bool ParallelTimeDirectories::latestTimeNeedsReconst() const
{
    if (proc0Times_.size())
    {
        auto lproc = (--proc0Times_.end());
        return proc0TimeDirNeedsReconst(lproc->second);
    }
    else
        return false;
}

bool ParallelTimeDirectories::isParallelTimeDirInconsistent(
    const boost::filesystem::path &ptd ) const
{
    auto j0=procDirs_.begin();
    if (!boost::filesystem::exists((*j0)/ptd))
    {
        return true;
    }
    else
    {
        auto tdc1 = OpenFOAMCaseDirs::timeDirContent((*j0)/ptd);
        for (auto j=++procDirs_.begin(); j!=procDirs_.end(); ++j)
        {
            if (!boost::filesystem::exists((*j)/ptd))
            {
                return true;
            }
            else
            {
                auto tdc2=OpenFOAMCaseDirs::timeDirContent((*j)/ptd);
                if ( tdc2 != tdc1 )
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void ParallelTimeDirectories::reconstructNewTimeDirs() const
{
    auto rtds = newParallelTimes();
    rtds.erase(boost::filesystem::path("0"));

    std::vector<std::string> tdbns;
    std::transform(rtds.begin(), rtds.end(), std::back_inserter(tdbns),
                   [&](const decltype(rtds)::value_type& path) { return path.string(); } );

    if (rtds.size())
    {
        cm_.executeCommand(
            location_,
            "reconstructPar",
            { "-time", boost::join(tdbns, ",") }
            );
    }
}



bool checkIfReconstructLatestTimestepNeeded
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location
)
{
    return ParallelTimeDirectories(cm, location)
        .latestTimeNeedsReconst();

//  using namespace boost::filesystem;
  
//  path proc0 = location/"processor0";
//  if (!exists(proc0))
//  {
//      std::cout<<"No processor directories in case "<<location.string()<<" => no reconstruct possible"<<std::endl;
//      return false; // no reconst, if no processor directories exist
//  }
  
//  // find last timestep in proc*0
//  TimeDirectoryList tdl = listTimeDirectories( proc0 );
//  boost::filesystem::path proc0latestTimeDir = tdl.rbegin()->second;
  
//  if (tdl.size()==0)
//  {
//      std::cout<<"No time directories in procesor0 => no reconstruct possible"<<std::endl;
//      return false; // no reconst, if not time dirs in proc dirs
//  }

//  path timedirname = proc0latestTimeDir.filename();
//  path latestTimeDir = location/timedirname;
  
//  if (!exists(latestTimeDir))
//  {
//      std::cout<<"Latest time directory "<<timedirname.string()<<" in procesor0 not existing in case "<<location.string()<<" => reconstruct required"<<std::endl;
//      return true; // reconst needed, if latest time is only existing in proc dir
//  }
//  else
//  {
//      // time dir exists also in case; check if files in proc dir are newer
      
//      if (checkIfAnyFileIsNewerOrNonexistent(proc0latestTimeDir, latestTimeDir, false))
//      {
//          std::cout<<"There are newer or non-existing field files in case time dir => reconstruct required"<<std::endl;
//          return true;
//      }
      
//      if (exists(proc0latestTimeDir/"polyMesh"))
//      {
//          if (!exists(latestTimeDir/"polyMesh"))
//          {
//            std::cout<<"polyMesh folder in case time dir not existing => reconstruct required"<<std::endl;
//            return true;
//          }
//          else if (checkIfAnyFileIsNewerOrNonexistent(proc0latestTimeDir/"polyMesh", latestTimeDir/"polyMesh", false))
//          {
//            std::cout<<"There are newer or non-existing files in polyMesh folder => reconstruct required"<<std::endl;
//            return true;
//          }
//      }
//  }
//  return false;
}




eMesh::eMesh()
{}

eMesh::eMesh(const EMeshPtsList &pts)
{
    for (int i=0; i<pts.size(); ++i)
    {
        points_.push_back(pts[i]);
        if (i>0) edges_.push_back({i-1, i});
    }
}

eMesh::eMesh(const EMeshPtsListList &pts)
{

    for (const EMeshPtsList& points: pts)
    {
      for (const arma::mat& p: points)
      {
        points_.push_back(p);
      }
    }

    int ofs=0;
    for (const EMeshPtsList& points: pts)
    {
      for (size_t i=1; i<points.size(); i++)
      {
        edges_.push_back({ofs+i-1, ofs+i});
      }
      ofs+=points.size();
    }
}

int eMesh::nPoints() const
{
    return points_.size();
}

int eMesh::nEdges() const
{
    return edges_.size();
}

void eMesh::write(std::ostream &f) const
{
    f<<"FoamFile {"<<endl
     <<" version     2.0;"<<endl
     <<" format      ascii;"<<endl
     <<" class       featureEdgeMesh;"<<endl
     <<" location    \"\";"<<endl
     <<" object      \"export.eMesh\";"<<endl
     <<"}"<<endl;

    f<<points_.size()<<endl
     <<"("<<endl;
    for (const arma::mat& p: points_)
    {
      f<<OFDictData::vector3(p)<<endl;
    }
    f<<")"<<endl;

    f<<edges_.size()<<endl
     <<"("<<endl;
    for (const auto&e: edges_)
    {
      f<<"("<<e.first<<" "<<e.second<<")"<<endl;
    }
    f<<")"<<endl;
}

void eMesh::write(const boost::filesystem::path& filename) const
{
    std::ofstream f(filename.string());
    write(f);
}

std::ostream &operator<<(std::ostream& os, const eMesh& emesh)
{
    emesh.write(os);
    return os;
}


void exportEMesh(const EMeshPtsList &points, const boost::filesystem::path& filename)
{
    eMesh(points).write(filename);

//  f<<"FoamFile {"<<endl
//   <<" version     2.0;"<<endl
//   <<" format      ascii;"<<endl
//   <<" class       featureEdgeMesh;"<<endl
//   <<" location    \"\";"<<endl
//   <<" object      "<<filename.filename().string()<<";"<<endl
//   <<"}"<<endl;

//  f<<points.size()<<endl
//   <<"("<<endl;
//  for (const arma::mat& p: points)
//  {
//    f<<OFDictData::to_OF(p)<<endl;
//  }
//  f<<")"<<endl;

//  f<<(points.size()-1)<<endl
//   <<"("<<endl;
//  for (size_t i=1; i<points.size(); i++)
//  {
//    f<<"("<<(i-1)<<" "<<i<<")"<<endl;
//  }
//  f<<")"<<endl;
}


void exportEMesh(const std::vector<EMeshPtsList>& pts, const boost::filesystem::path& filename)
{
    eMesh(pts).write(filename);

//  f<<"FoamFile {"<<endl
//   <<" version     2.0;"<<endl
//   <<" format      ascii;"<<endl
//   <<" class       featureEdgeMesh;"<<endl
//   <<" location    \"\";"<<endl
//   <<" object      "<<filename.filename().string()<<";"<<endl
//   <<"}"<<endl;

//  int npts=0;
//  for (const EMeshPtsList& points: pts)
//  {
//    npts+=points.size();
//  }
//  f<<npts<<endl
//   <<"("<<endl;
//  for (const EMeshPtsList& points: pts)
//  {
//    for (const arma::mat& p: points)
//    {
//      f<<OFDictData::to_OF(p)<<endl;
//    }
//  }
//  f<<")"<<endl;

//  f<<(npts-pts.size())<<endl
//   <<"("<<endl;
//  int ofs=0;
//  for (const EMeshPtsList& points: pts)
//  {
//    for (size_t i=1; i<points.size(); i++)
//    {
//      f<<"("<<(ofs+i-1)<<" "<<(ofs+i)<<")"<<endl;
//    }
//    ofs+=points.size();
//  }
//  f<<")"<<endl;
}


OpenFOAMCaseDirs::OpenFOAMCaseDirs
(
    const OpenFOAMCase&,
    const path& location
)
  : location_(location)
{
  if (!exists(location))
    throw insight::Exception("OpenFOAM case location does not exist: "+location.string());

  std::vector<path> to_test;

  to_test = { "system", "constant" };
  for (const auto& tt: to_test)
  {
    if (exists(location/tt)) sysDirs_.insert(location/tt);
  }
  to_test = { "wnow", "wnowandstop" };
  for (const auto& tt: to_test)
  {
    if (exists(location/tt)) sysDirs_.insert(location/tt);
  }

  to_test = { "postProcessing", "VTK" };
  for (const auto& tt: to_test)
  {
    if (exists(location/tt)) postDirs_.insert(location/tt);
  }

  TimeDirectoryList tdl = listTimeDirectories(location);
  for (const auto& td: tdl)
  {
    timeDirs_.push_back(td.second);
  }



  directory_iterator end_itr; // default construction yields past-the-end
  const boost::regex filter( "processor[0-9]+" );
  for ( directory_iterator i( location ); i != end_itr; i++ )
  {
      if ( is_directory(i->status()) )
      {
          std::string fn=i->path().filename().string();
          boost::smatch what;
          if ( boost::regex_match( i->path().filename().string(), what, filter ) )
            {
              procDirs_.insert(i->path());
            }
      }
  }

  // get a list of every time directory processor dirs
  // also include potentially inconsistent dirs
  for (const auto& proc: procDirs_)
  {
      auto ptds = listTimeDirectories(proc);
      for (const auto& ptd: ptds)
      {
          procTimeDirs_.insert(ptd.second.filename());
      }
  }

}

std::set<boost::filesystem::path> OpenFOAMCaseDirs::timeDirs( OpenFOAMCaseDirs::TimeDirOpt td )
{
  std::set<boost::filesystem::path> tds;

  if (td==TimeDirOpt::All)
  {
    std::copy( timeDirs_.begin(), timeDirs_.end(), std::inserter(tds, tds.begin()) );
  }

  if ( td==TimeDirOpt::OnlyFirst || td==TimeDirOpt::OnlyFirstAndLast )
  {
    if (timeDirs_.size()>0) tds.insert(timeDirs_.front());
  }

  if (td==TimeDirOpt::OnlyLast || td==TimeDirOpt::OnlyFirstAndLast )
  {
    if (timeDirs_.size()>0)
    {
      if (timeDirs_.back().filename().string()!="0")
        tds.insert(timeDirs_.back());
    }
  }

  if (td==TimeDirOpt::ExceptFirst)
  {
    if (timeDirs_.size()>1)
    {
      std::copy( timeDirs_.begin()+1, timeDirs_.end(), std::inserter(tds, tds.begin()) );
    }
  }


  return tds;
}


std::set<boost::filesystem::path> OpenFOAMCaseDirs::timeDirContent(
        const boost::filesystem::path& td )
{
    std::set<boost::filesystem::path> tdc;

    for (const auto& f : recursive_directory_iterator(td))
    {
        tdc.insert(make_relative(td,f));
    }
    return tdc;
}

std::ostream& operator<<(std::ostream& os, const std::set<boost::filesystem::path>& paths)
{
    for (const auto& p: paths)
    {
        os << " "<<p.string();
    }
    return os;
}

std::set<boost::filesystem::path> OpenFOAMCaseDirs::caseFilesAndDirs
(
    OpenFOAMCaseDirs::TimeDirOpt td,
    bool cleanProc,
    bool cleanTimes,
    bool cleanPost,
    bool cleanSys,
    bool cleanInconsistentParallelTimes
)
{
  std::set<boost::filesystem::path> all_cands;

  if (cleanSys) std::copy( sysDirs_.begin(), sysDirs_.end(), std::inserter(all_cands, all_cands.begin()) );

  if (cleanPost) std::copy( postDirs_.begin(), postDirs_.end(), std::inserter(all_cands, all_cands.begin()) );

  if (cleanTimes)
  {
    auto tds = timeDirs(td);
    std::copy( tds.begin(), tds.end(), std::inserter(all_cands, all_cands.begin()) );
  }

  if (cleanProc) std::copy( procDirs_.begin(), procDirs_.end(), std::inserter(all_cands, all_cands.begin()) );

  if (cleanInconsistentParallelTimes)
  {
      std::set<boost::filesystem::path> inconsistentParTimes;

      auto j0=procDirs_.begin();
      for (const auto& ptd: procTimeDirs_)
      {
          if (!boost::filesystem::exists((*j0)/ptd))
          {
            inconsistentParTimes.insert(ptd);
          }
          else
          {
              auto tdc1 = timeDirContent((*j0)/ptd);
              for (auto j=++procDirs_.begin(); j!=procDirs_.end(); ++j)
              {
                  if (!boost::filesystem::exists((*j)/ptd))
                  {
                    inconsistentParTimes.insert(ptd);
                    break;
                  }
                  else
                  {
                      auto tdc2=timeDirContent((*j)/ptd);
                      if ( tdc2 != tdc1 )
                      {
                        inconsistentParTimes.insert(ptd);
                        break;
                      }
                  }
              }
          }
      }

      for (const auto& ptd: inconsistentParTimes)
      {
          for (const auto& pd: procDirs_)
          {
              auto dn = pd/ptd;
              if (boost::filesystem::exists(dn))
              {
                  all_cands.insert(dn);
              }
          }
      }
  }

  return all_cands;
}

void OpenFOAMCaseDirs::packCase(const boost::filesystem::path& archive_file,OpenFOAMCaseDirs::TimeDirOpt td)
{
  std::string cmd;
  cmd+="cd "+location_.string()+";";
  cmd+="tar czf "+archive_file.string();

  std::vector<std::string> filesAndDirsToPack;
  for (const auto& c: sysDirs_) filesAndDirsToPack.push_back(make_relative(location_, c).string());
  for (const auto& c: postDirs_) filesAndDirsToPack.push_back(make_relative(location_, c).string());

  auto tds = timeDirs(td);
  for (const auto& c: tds) filesAndDirsToPack.push_back(make_relative(location_, c).string());

  if (filesAndDirsToPack.size()>0)
  {

    cmd+=" "+boost::join(filesAndDirsToPack, " " );

    if (::system(cmd.c_str()) != 0)
      throw insight::Exception("Could not pack OpenFOAM case files.\n"
                               "Command was \""+cmd+"\"");
  }
  else
  {
    insight::Warning("There are no files or directories to pack. Nothing archived.");
  }
}

void OpenFOAMCaseDirs::cleanCase
(
    OpenFOAMCaseDirs::TimeDirOpt td,
    bool cleanProc,
    bool cleanTimes,
    bool cleanPost,
    bool cleanSys,
    bool cleanInconsistentParallelTimes
)
{

  auto cands = caseFilesAndDirs(td, cleanProc, cleanTimes, cleanPost, cleanSys, cleanInconsistentParallelTimes);

  for (const auto& c: cands)
  {
    std::cout<<"DELETING: "<<c<<std::endl;
    remove_all(c);
  }
}


void VTKFieldToOpenFOAMField(const boost::filesystem::path& vtkfile, const std::string& fieldname, std::ostream& out)
{
  vtkSmartPointer<vtkPolyDataReader> in = vtkPolyDataReader::New();
  in->SetFileName(vtkfile.string().c_str());
  in->ReadAllScalarsOn();
  in->ReadAllVectorsOn();
  in->ReadAllTensorsOn();
  in->Update();

  if(in->IsFilePolyData())
  {
    vtkPolyData* pd = in->GetOutput();

    if (!pd)
      throw insight::Exception("Error reading VTK file "+vtkfile.string());

    vtkDataArray* da = pd->GetCellData()->GetArray(fieldname.c_str());

    if (!da)
    {
      int na=pd->GetCellData()->GetNumberOfArrays();
      std::ostringstream m;
      m<<"Error accessing cell field \""<<fieldname<<"\" in file "<<vtkfile.string()<<"!\n";
      m<<"Available arrays: (";
      for (int k=0; k<na; k++)
      {
        m<<" "<<pd->GetCellData()->GetArrayName(k);
      }
      m<<" )";
      throw insight::Exception(m.str());
    }

    vtkIdType ncells=da->GetNumberOfTuples();
    vtkIdType nc=da->GetNumberOfComponents();

    out << ncells << "\n(\n";
    for (vtkIdType i=0; i<ncells; i++)
    {
      if (nc>1) out<<" (";
      double *cd = da->GetTuple(i);
      for (vtkIdType j=0; j<nc; j++) out<<" "<<cd[j];
      if (nc>1) out<<" )";
      out<<'\n';
    }
    out << ")\n";
  }
}

decompositionState::decompositionState(const boost::filesystem::path& casedir)
{
  CurrentExceptionContext ce("Checking decomposition state of case in "+casedir.string());

  if (!boost::filesystem::exists(casedir))
    throw insight::Exception("Case directory "+casedir.string()+" does not exist!");

  int np=1;
  try
  {
    np=readDecomposeParDict(casedir);
  }
  catch (...) {} // cannot read decomposeParDict (not existent) => serial case

  std::vector<boost::filesystem::path> procDirs;
  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( casedir );
          itr != end_itr; itr++ )
  {
      if ( is_directory(itr->status()) )
      {
          if (starts_with(itr->path().filename().string(), "processor"))
          {
            procDirs.push_back(itr->path());
          }
      }
  }

  if (procDirs.size()>0)
    hasProcessorDirectories=true;
  else
    hasProcessorDirectories=false;

  if (procDirs.size()==size_t(np))
    nProcDirsMatchesDecomposeParDict=true;
  else
    nProcDirsMatchesDecomposeParDict=false;

  //
  // check, if the same latest time step is existing in all processor directories and
  // has everywhere the same fields
  //
  decomposedLatestTimeIsConsistent=true;
  std::string latestTime;
  std::set<std::string> fields0;
  for (auto pd=procDirs.begin(); pd!=procDirs.end(); pd++)
  {
    auto tdl = listTimeDirectories(*pd);

    if (pd==procDirs.begin())
    {
        if (tdl.size()>0)
        {
          latestTime=tdl.rbegin()->second.filename().string();
          for (const auto& f:
               boost::filesystem::directory_iterator(tdl.rbegin()->second))
          {
            // get list of fields
            fields0.insert( f.path().filename().string() );
          }
        }
    }
    else
    {
      if (tdl.size()>0)
      {
        std::string cur_latestTime=tdl.rbegin()->second.filename().string();

        if (cur_latestTime!=latestTime)
        {
          decomposedLatestTimeIsConsistent = false;
        }
        else
        {
          size_t nfound=0;
          // check, if the same fields are present
          for (const auto& f: boost::filesystem::directory_iterator(tdl.rbegin()->second))
          {
            if (fields0.find(f.path().filename().string()) != fields0.end())
            {
              nfound++;
            }
          }
          decomposedLatestTimeIsConsistent = decomposedLatestTimeIsConsistent
                                             && (nfound==fields0.size());
        }
      }
    }
  }

  if (!hasProcessorDirectories)
  {
    laterLatestTime=Location::Reconstructed;
    newerFiles=Location::Reconstructed;
  }
  else
  {
    if (decomposedLatestTimeIsConsistent)
    {
      // check, where the latest time step lies
      auto tdl = listTimeDirectories(casedir);
      if (tdl.size()==0)
      {
        if (latestTime==std::string())
        {
          // neither in decomposed nor reconst case are time steps
          laterLatestTime=Location::Both;
          newerFiles=Location::Undefined;
        }
        else
        {
          // no reconst time steps but time steps in proc dirs
          laterLatestTime=Location::Decomposed;
          newerFiles=Location::Decomposed;
        }
      }
      else
      {
        if (latestTime==std::string())
        {
          // in reconst case are time steps but not in proc dirs
          laterLatestTime=Location::Reconstructed;
          newerFiles=Location::Reconstructed;
        }
        else
        {
          double lt_decomp=toNumber<double>(latestTime);
          if ( fabs(tdl.rbegin()->first - lt_decomp) < 1e-15 )
          {
            // same time step in recon and decomp
            laterLatestTime=Location::Both;

            boost::filesystem::path
                p_dec = casedir/"processor0"/latestTime,
                p_rec = tdl.rbegin()->second;
            bool rec_newer=false, dec_newer=false;
            for (const auto& f: fields0)
            {
              if (
                  boost::filesystem::last_write_time(p_dec/f)
                  <
                  boost::filesystem::last_write_time(p_rec/f)
                 )
              {
                rec_newer=true;
              }
              else
              {
                dec_newer=true;
              }
            }
            if (rec_newer && dec_newer)
            {
              newerFiles=Location::Both;
            }
            else if (rec_newer)
            {
              newerFiles=Location::Reconstructed;
            }
            else if (dec_newer)
            {
              newerFiles=Location::Decomposed;
            }
            else
            {
              newerFiles=Location::Undefined;
            }
          }
          else if ( tdl.rbegin()->first > lt_decomp )
          {
            laterLatestTime=Location::Reconstructed;
            newerFiles=Location::Reconstructed;
          }
          else if ( tdl.rbegin()->first < lt_decomp )
          {
            laterLatestTime=Location::Decomposed;
            newerFiles=Location::Decomposed;
          }
        }
      }
    }
  }

}


BoundingBox::BoundingBox()
  : arma::mat(arma::zeros(3,2))
{
}

void BoundingBox::extend(const arma::mat& bb2)
{
  arma::mat& bb = (*this);
  for (arma::uword i=0; i<3; i++)
  {
   bb(i,0)=std::min( bb(i,0), bb2(i,0));
   bb(i,1)=std::max( bb(i,1), bb2(i,1));
  }
}

void BoundingBox::operator=(const arma::mat& bb)
{
    arma::mat::operator=(bb);
}

void setHydrostaticPressure(
    const OpenFOAMCase &cm,
    const boost::filesystem::path &location,
    const arma::mat &pSurf,
    const arma::mat &eUp,
    double rho, double p0Amb,
    const std::map<std::string, std::string> &targetEntriesPerPatch,
    const std::string& fieldName,
    bool setInternalField )
{
    std::string expr = str(
        boost::format("%g - (pos()-vector(%g,%g,%g))&vector(%g,%g,%g)*%g*9.81")
            % p0Amb
            % pSurf(0) % pSurf(1) % pSurf(2)
            % eUp(0) % eUp(1) % eUp(2)
            % rho
        );

    if (setInternalField)
    {
        OFDictData::dictFile sefd;
        sefd["defaultFieldValues"]=
            OFDictData::list{str(boost::format("volScalarFieldValue "+fieldName+" %g")%p0Amb)};

        OFDictData::dict p;
        p["field"]=fieldName;
        p["constants"]=OFDictData::dict();
        p["variables"]=OFDictData::list();
        p["expression"]="#{"+expr+"#}";
        sefd["expressions"]=OFDictData::list{ fieldName, p };

        sefd.write( location / "system" / "setExprFieldsDict" );

        cm.executeCommand(location, "setExprFields", {});
    }

    if (targetEntriesPerPatch.size())
    {
        OFDictData::dict pat;
        pat["field"]=fieldName;
        pat["keepPatches"]=true;

        auto addExpr = [&](const std::string& patch, const std::string& targetEntry)
        {
            OFDictData::dict exprDict;
            exprDict["patch"]=patch;
            exprDict["target"]=targetEntry;
            exprDict["expression"]="#{ "+expr+" #}";
            return exprDict;
        };

        OFDictData::dict boundaryDict;
        cm.parseBoundaryDict(location, boundaryDict);

        OFDictData::list exprs;
        for (const auto& tepp: targetEntriesPerPatch)
        {
            boost::regex re(tepp.first);
            for (const auto& p: boundaryDict)
            {
                if (boost::regex_match(p.first, re))
                {
                    exprs.push_back(addExpr(p.first, tepp.second));
                }
            }
        }
        pat["expressions"]=exprs;

        OFDictData::dictFile sebfd;
        sebfd["pattern"]=pat;

        sebfd.write( location / "system" / "setExprBoundaryFieldsDict" );

        cm.executeCommand(location, "setExprBoundaryFields", {});
    }
}





}
