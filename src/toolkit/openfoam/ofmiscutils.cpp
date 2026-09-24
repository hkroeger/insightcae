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

#include "ofmiscutils.h"
#include "openfoam/oftimedirectories.h"
#include "openfoam/ofmeshcopy.h"
#include "openfoam/ofpatchutils.h"
#include "openfoam/ofdictreaders.h"
#include "base/boundingbox.h"
#include "base/resultelements/comment.h"
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


void createCellZoneFromRegionSeedPoint(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& dir,
    const std::string& zoneName,
    const arma::mat& PiM,
    const std::vector<std::string>& asc
    )
{
    std::string pims=OFDictData::toString(OFDictData::vector3(PiM)), nErode="";
    if (cm.OFversion()>=220) pims="("+pims+")";
    if (cm.OFversion()>=400) nErode=" 0";

    std::vector<std::string> setCmds={
        "cellSet dummy new boxToCell (-1e10 -1e10 -1e10) (1e10 1e10 1e10)",
        "cellSet "+zoneName+" new regionToCell dummy "+pims+nErode,
        "cellSet dummy remove"
    };

    std::copy(
        asc.begin(), asc.end(),
        std::last_inserter(setCmds)
        );

    setSet(cm, dir, setCmds);
    cm.executeCommand(dir, "setsToZones", { "-noFlipMap" } );
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
  potentialFlow["PhiRefCell"]=0;
  potentialFlow["PhiRefValue"]=0.;
  
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
             {"-noFunctionObjects"} );

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

void currentNumericalSettingsReport
(
  const OpenFOAMCase& /*cm*/,
  const boost::filesystem::path& location,
  ResultSet& results
)
{
  double order=990;
  for
  (
    const boost::filesystem::path& dictname:
    {
      "system/controlDict", "system/fvSchemes", "system/fvSchemes",
      "constant/RASProperties", "constant/LESProperties",
      "constant/transportProperties" }
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
      results.insert("dictionary_"+elemname,
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
  opts.push_back(toString(n));
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

void surfaceFeatureExtract
(
  const OpenFOAMCase& cm, 
  const boost::filesystem::path& location,
  const std::string& surfaceName,
  double featureAngle
)
{
  OFDictData::dictFile sfeDict;
  
  OFDictData::dict opts;
  
  opts["extractionMethod"]="extractFromSurface";
  
  OFDictData::dict coeffs;
  coeffs["includedAngle"]=180.-featureAngle; //120.0;
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
  opts.push_back(toString(n));
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

BoundingBox::BoundingBox()
    : arma::mat(initializedBndBox())
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

void initializeTurbulenceFields(
    OpenFOAMCase &cm,
    const boost::filesystem::path& dir )
{
    std::vector<std::string> opts;
    for (auto fld: {"k", "epsilon", "omega"})
    {
        if (cm.hasField(fld))
            opts.push_back(std::string("-")+fld);
    }
    if (opts.size())
    {
        opts.insert(opts.begin(), {"0.05", "0.1"});
        cm.executeCommand(
            dir,
            "initializeTurbulenceFields", opts);
    }
}


}
