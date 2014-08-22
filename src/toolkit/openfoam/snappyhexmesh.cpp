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

#include "snappyhexmesh.h"

#include "boost/filesystem.hpp"
#include "boost/assign.hpp"
#include "openfoam/openfoamtools.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
namespace snappyHexMeshFeats
{

void Feature::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
}

Geometry::Geometry( Parameters const& p )
: p_(p)
{
}

void Geometry::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p_.name();
  //boost::filesystem::path x; x.f
  sHMDict.subDict("geometry")[p_.fileName().filename().c_str()]=geodict;

  OFDictData::dict castdict;
  OFDictData::list levels;
  levels.push_back(p_.minLevel());
  levels.push_back(p_.maxLevel());
  castdict["level"]=levels;
  sHMDict.subDict("castellatedMeshControls").subDict("refinementSurfaces")[p_.name()]=castdict;

  OFDictData::dict layerdict;
  layerdict["nSurfaceLayers"]=p_.nLayers();
  sHMDict.subDict("addLayersControls").subDict("layers")["\""+p_.name()+".*\""]=layerdict;

}

void Geometry::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
  boost::filesystem::path from(p_.fileName());
  boost::filesystem::path to(location/"constant"/"triSurface"/p_.fileName().filename());
  if (!exists(to.parent_path()))
    create_directories(to.parent_path());
  //copy_file(from, to);
  ofc.executeCommand(location, "surfaceTransformPoints",
    list_of<std::string>
    (absolute(from).string())
    (absolute(to).string())
    ("-scale")(OFDictData::to_OF(p_.scale()))
//     ("("+lexical_cast<std::string>(p_.scale()(0))+" "+lexical_cast<std::string>(p_.scale()(1))+" "+lexical_cast<std::string>(p_.scale()(2))+")")
    ("-translate")(OFDictData::to_OF(p_.translate()))
    ("-rollPitchYaw")(OFDictData::to_OF(p_.rollPitchYaw()))
//     ("("+lexical_cast<std::string>(p_.rollPitchYaw()(0))+" "+lexical_cast<std::string>(p_.rollPitchYaw()(1))+" "+lexical_cast<std::string>(p_.rollPitchYaw()(2))+")")
  );
}

Feature* Geometry::clone() const
{
  return new Geometry(p_);
}

RefinementRegion::RefinementRegion(Parameters const& p)
: p_(p)
{
}

void RefinementRegion::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  if (setGeometrySubdict(geodict))
    sHMDict.subDict("geometry")[p_.name()]=geodict;

  OFDictData::dict castdict;
  castdict["mode"]=p_.mode();
  OFDictData::list level;
  level.push_back(p_.distance());
  level.push_back(p_.level());
  OFDictData::list levels;
  levels.push_back(level);
  castdict["levels"]=levels;
  sHMDict.subDict("castellatedMeshControls").subDict("refinementRegions")[p_.name()]=castdict;
}


RefinementBox::RefinementBox(const RefinementBox::Parameters& p)
: RefinementRegion(p),
  p_(p)
{
}

bool RefinementBox::setGeometrySubdict(OFDictData::dict& d) const
{
  d["type"]="searchableBox";
  d["min"]=OFDictData::to_OF(p_.min());
  d["max"]=OFDictData::to_OF(p_.max());
  return true;
}

Feature* RefinementBox::clone() const
{
  return new RefinementBox(p_);
}

NearSurfaceRefinement::NearSurfaceRefinement(const RefinementRegion::Parameters& p)
: RefinementRegion(p)
{
}

bool NearSurfaceRefinement::setGeometrySubdict(OFDictData::dict& d) const
{
  // do nothing
  return false;
}


Feature* NearSurfaceRefinement::clone() const
{
  return new NearSurfaceRefinement(p_);
}


}

void setStdCastellatedCtrls(OFDictData::dict& castellatedCtrls)
{
  castellatedCtrls["maxLocalCells"]=1000000;
  castellatedCtrls["maxGlobalCells"]=10000000;
  castellatedCtrls["minRefinementCells"]=10;
  castellatedCtrls["maxLoadUnbalance"]=0.1;
  castellatedCtrls["nCellsBetweenLevels"]=2;
  castellatedCtrls["resolveFeatureAngle"]=30.0;
  castellatedCtrls["allowFreeStandingZoneFaces"]=true;
}

void setStdSnapCtrls(OFDictData::dict& snapCtrls)
{
  snapCtrls["nSmoothPatch"]=3;
  snapCtrls["tolerance"]=4.0;
  snapCtrls["nSolveIter"]=30;
  snapCtrls["nRelaxIter"]=5;  

  snapCtrls["nFeatureSnapIter"]=10;  
  snapCtrls["implicitFeatureSnap"]=false;  
  snapCtrls["explicitFeatureSnap"]=true;  
  snapCtrls["multiRegionFeatureSnap"]=false;  

}

void setStdLayerCtrls(OFDictData::dict& layerCtrls)
{
  layerCtrls["relativeSizes"]=true;
  layerCtrls["expansionRatio"]=1.3;
  layerCtrls["finalLayerThickness"]=0.5;
  layerCtrls["minThickness"]=1e-5;  
  layerCtrls["nGrow"]=0;  
  layerCtrls["featureAngle"]=30.0;  

  layerCtrls["slipFeatureAngle"]=30.0;  

  layerCtrls["nRelaxIter"]=10;  
  layerCtrls["nSmoothSurfaceNormals"]=1;  
  layerCtrls["nSmoothNormals"]=3;  
  layerCtrls["nSmoothThickness"]=10;  
  layerCtrls["maxFaceThicknessRatio"]=200.0;  
  layerCtrls["maxThicknessToMedialRatio"]=0.5;  
  layerCtrls["minMedianAxisAngle"]=130.0;  
  layerCtrls["nBufferCellsNoExtrude"]=0;  
  layerCtrls["nLayerIter"]=2;  //OCFD
  layerCtrls["maxLayerIter"]=2;  // engys
}

void setStdQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=85.0;
  qualityCtrls["maxBoundarySkewness"]=20.0;
  qualityCtrls["maxInternalSkewness"]=4.0;
  qualityCtrls["maxConcave"]=85.0;  
  qualityCtrls["minFlatness"]=0.002;  
  qualityCtrls["minVol"]=1e-18;  
  qualityCtrls["minArea"]=-1.0;  
  qualityCtrls["minTwist"]=0.001;  
  qualityCtrls["minDeterminant"]=0.00001;  
  qualityCtrls["minFaceWeight"]=0.001;  
  qualityCtrls["minVolRatio"]=0.0005;  
  qualityCtrls["minTriangleTwist"]=-1.0;  
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  

  qualityCtrls["minTetQuality"]=1e-40;  
}


void snappyHexMesh(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location,
		  const OFDictData::list& PiM,
		  const boost::ptr_vector<snappyHexMeshFeats::Feature>& ops,
		  snappyHexMeshOpts::Parameters const& p,
		  bool overwrite
		  )
{
  using namespace snappyHexMeshFeats;
  
  OFDictData::dictFile sHMDict;
  
  // setup dict structure
  sHMDict["castellatedMesh"] = true;
  sHMDict["snap"] = true;
  sHMDict["addLayers"] = true;
  sHMDict["debug"] = 0;
  sHMDict["mergeTolerance"] = 1e-6;
  OFDictData::dict& geomCtrls=sHMDict.addSubDictIfNonexistent("geometry");
  OFDictData::dict& castellatedCtrls = sHMDict.addSubDictIfNonexistent("castellatedMeshControls");
  castellatedCtrls.addListIfNonexistent("features");
  castellatedCtrls.addSubDictIfNonexistent("refinementSurfaces");
  castellatedCtrls.addSubDictIfNonexistent("refinementRegions");
  OFDictData::dict& snapCtrls=sHMDict.addSubDictIfNonexistent("snapControls");
  OFDictData::dict& layerCtrls=sHMDict.addSubDictIfNonexistent("addLayersControls");
  layerCtrls.addSubDictIfNonexistent("layers");
  OFDictData::dict& qualityCtrls=sHMDict.addSubDictIfNonexistent("meshQualityControls");

  //  populate with defaults
  setStdSnapCtrls(snapCtrls);
  setStdCastellatedCtrls(castellatedCtrls);
  castellatedCtrls["locationInMesh"]=PiM;
  setStdLayerCtrls(layerCtrls);
  layerCtrls["relativeSizes"]=p.relativeSizes();
  layerCtrls["finalLayerThickness"]=p.tlayer();
  layerCtrls["expansionRatio"]=p.erlayer();
  layerCtrls["nLayerIter"]=p.nLayerIter();  //OCFD
  layerCtrls["maxLayerIter"]=p.nLayerIter();  // engys
  setStdQualityCtrls(qualityCtrls);

  BOOST_FOREACH( const snappyHexMeshFeats::Feature& feat, ops)
  {
    feat.modifyFiles(ofc, location);
    feat.addIntoDictionary(sHMDict);
  }
  
  // then write to file
  boost::filesystem::path dictpath = location / "system" / "snappyHexMeshDict";
  if (!exists(dictpath.parent_path())) 
  {
    boost::filesystem::create_directories(dictpath.parent_path());
  }
  
  {
    std::ofstream f(dictpath.c_str());
    writeOpenFOAMDict(f, sHMDict, boost::filesystem::basename(dictpath));
  }

  std::vector<std::string> opts;
  if (overwrite) opts.push_back("-overwrite");
        
  int np=readDecomposeParDict(location);
  bool is_parallel = (np>1);

  if (is_parallel)
  {
    ofc.executeCommand(location, "decomposePar");
  }
  
  //cm.runSolver(executionPath(), analyzer, solverName, &stopFlag_, np);
  ofc.executeCommand(location, "snappyHexMesh", opts, NULL, np);
  
  if (is_parallel)
  {
    ofc.executeCommand(location, "reconstructParMesh", list_of<string>("-constant") );
    ofc.removeProcessorDirectories(location);
  }
  
  
  //ofc.executeCommand(location, "snappyHexMesh", opts);
}
  
}
