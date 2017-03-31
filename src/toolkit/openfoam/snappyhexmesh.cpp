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

#include "base/boost_include.h"
#include "openfoam/openfoamtools.h"
#include "base/exception.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
enum trimmedMesher {sHM, cfM};


ExternalGeometryFile::ExternalGeometryFile(const ParameterSet& ps)
: p_(ps)
{}

std::string ExternalGeometryFile::fileName() const
{
    return p_.fileName.filename().string();
}


void ExternalGeometryFile::putIntoConstantTrisurface(const OpenFOAMCase& ofc, const path& location) const
{
  boost::filesystem::path from(p_.fileName);
  boost::filesystem::path to(location/"constant"/"triSurface"/p_.fileName.filename());
  
  if (!exists(to.parent_path()))
    create_directories(to.parent_path());

  ofc.executeCommand(location, "surfaceTransformPoints",
    list_of<std::string>
    (absolute(from).string())
    (absolute(to).string())
    ("-scale")(OFDictData::to_OF(p_.scale))
    ("-translate")(OFDictData::to_OF(p_.translate))
    ("-rollPitchYaw")(OFDictData::to_OF(p_.rollPitchYaw))
  );
}

  
  
  
namespace snappyHexMeshFeats
{

    
    
    
defineType(Feature);
defineDynamicClass(Feature);
    
void Feature::modifyFiles
(
  const OpenFOAMCase&,
  const boost::filesystem::path&
) const
{
}



defineType(Geometry);
addToFactoryTable(Feature, Geometry);
addToStaticFunctionTable(Feature, Geometry, defaultParameters);


Geometry::Geometry( const ParameterSet& ps )
: ExternalGeometryFile(ps),
  p_(ps)
{
}

void Geometry::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p_.name;
    //boost::filesystem::path x; x.f
  sHMDict.subDict("geometry")[p_.fileName.filename().c_str()]=geodict;

  OFDictData::dict castdict;
  OFDictData::list levels;
  levels.push_back(p_.minLevel);
  levels.push_back(p_.maxLevel);
  castdict["level"]=levels;
  if (p_.zoneName!="")
  {
    castdict["faceZone"]=p_.zoneName;
    castdict["cellZone"]=p_.zoneName;
    castdict["cellZoneInside"]="inside";
  }
  if (p_.regionRefinements.size()>0)
  {
    OFDictData::dict rrd;
    BOOST_FOREACH(const Parameters::regionRefinements_default_type& rr, p_.regionRefinements)
    {
      OFDictData::dict ld;
      OFDictData::list rrl;
      rrl.push_back(rr.minLevel);
      rrl.push_back(rr.maxLevel);
      ld["level"]=rrl;
      //rrd[p_.name()+"_"+boost::get<0>(rr)] = ld;
      rrd[rr.regionname] = ld;
    }
    castdict["regions"]=rrd;
  }
  sHMDict.subDict("castellatedMeshControls").subDict("refinementSurfaces")[p_.name]=castdict;

  OFDictData::dict layerdict;
  layerdict["nSurfaceLayers"]=p_.nLayers;
  sHMDict.subDict("addLayersControls").subDict("layers")["\""+p_.name+".*\""]=layerdict;
  BOOST_FOREACH(const Parameters::regionRefinements_default_type& rr, p_.regionRefinements)
  {
   OFDictData::dict layerdict;
   layerdict["nSurfaceLayers"]=0;
   sHMDict.subDict("addLayersControls").subDict("layers")[p_.name+"_"+rr.regionname]=layerdict;
  }

}

void Geometry::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
  ExternalGeometryFile::putIntoConstantTrisurface(ofc, location);
}




defineType(PatchLayers);
addToFactoryTable(Feature, PatchLayers);
addToStaticFunctionTable(Feature, PatchLayers, defaultParameters);

PatchLayers::PatchLayers(const ParameterSet& ps)
: p_(ps)
{
}


void PatchLayers::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict layerdict;
  layerdict["nSurfaceLayers"]=p_.nLayers;
  sHMDict.subDict("addLayersControls").subDict("layers")[p_.name]=layerdict;
}




defineType(ExplicitFeatureCurve);
addToFactoryTable(Feature, ExplicitFeatureCurve);
addToStaticFunctionTable(Feature, ExplicitFeatureCurve, defaultParameters);


ExplicitFeatureCurve::ExplicitFeatureCurve( const ParameterSet& ps )
: p_(ps)
{
}

void ExplicitFeatureCurve::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict refdict;
  refdict["file"]=std::string("\"")+p_.fileName.filename().c_str()+"\"";
  refdict["level"]=p_.level;
  sHMDict.subDict("castellatedMeshControls").addListIfNonexistent("features").push_back(refdict);

}

void ExplicitFeatureCurve::modifyFiles(const OpenFOAMCase& ofc, const path& location) const
{
  boost::filesystem::path from(p_.fileName);
  if (!exists(from))
  {
    boost::filesystem::path alt_from=from; alt_from.replace_extension(".eMesh.gz");
    if (exists(alt_from)) from=alt_from;
    else
      throw insight::Exception("feature edge file does not exist: neither "+from.string()+" nor "+alt_from.string());
  }
  boost::filesystem::path to(location/"constant"/"triSurface"/from.filename());
  
  if (to!=from) // might occur, if file path in target location is specified (e.g. after surfaceFeatureExtract)
  {
    if (!exists(to.parent_path()))
      create_directories(to.parent_path());
    
    std::cout<<"copy from "<<from<<" to "<<to<<std::endl;
    copy_file(from, to, copy_option::overwrite_if_exists);
  }
//   ofc.executeCommand(location, "surfaceTransformPoints",
//     list_of<std::string>
//     (absolute(from).string())
//     (absolute(to).string())
//     ("-scale")(OFDictData::to_OF(p_.scale()))
//     ("-translate")(OFDictData::to_OF(p_.translate()))
//     ("-rollPitchYaw")(OFDictData::to_OF(p_.rollPitchYaw()))
//   );
}



defineType(RefinementRegion);

RefinementRegion::RefinementRegion(const ParameterSet& ps)
: p_(ps)
{
}

void RefinementRegion::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  std::string entryTitle=p_.name;
  if (setGeometrySubdict(geodict, entryTitle))
    sHMDict.subDict("geometry")[entryTitle]=geodict;

  OFDictData::dict castdict;
  std::string mode;
  if ( p_.mode == Parameters::inside )
  {
      mode="inside";
  }
  else if (p_.mode==Parameters::outside)
  {
      mode="outside";
  }
  else if (p_.mode == Parameters::distance)
  {
      mode="distance";
  }
  castdict["mode"]=mode;
  OFDictData::list level;
  level.push_back(p_.dist);
  level.push_back(p_.level);
  OFDictData::list levels;
  levels.push_back(level);
  castdict["levels"]=levels;
  sHMDict.subDict("castellatedMeshControls").subDict("refinementRegions")[p_.name]=castdict;
}




defineType(RefinementBox);
addToFactoryTable(Feature, RefinementBox);
addToStaticFunctionTable(Feature, RefinementBox, defaultParameters);

RefinementBox::RefinementBox(const ParameterSet& ps)
: RefinementRegion(ps),
  p_(ps)
{
}

bool RefinementBox::setGeometrySubdict(OFDictData::dict& d, std::string&) const
{
  d["type"]="searchableBox";
  d["min"]=OFDictData::to_OF(p_.min);
  d["max"]=OFDictData::to_OF(p_.max);
  return true;
}




defineType(RefinementGeometry);
addToFactoryTable(Feature, RefinementGeometry);
addToStaticFunctionTable(Feature, RefinementGeometry, defaultParameters);

RefinementGeometry::RefinementGeometry(const ParameterSet& ps)
: RefinementRegion(ps),
  p_(ps),
  geometryfile_(p_.geometry)
{}

// void RefinementGeometry::addIntoDictionary(OFDictData::dict& sHMDict) const
// {
//   OFDictData::dict geodict;
//   if (setGeometrySubdict(geodict))
//     sHMDict.subDict("geometry")[p_.fileName().filename().c_str()]=geodict;
// 
//   OFDictData::dict castdict;
//   castdict["mode"]=p_.mode();
//   OFDictData::list level;
//   level.push_back(p_.distance());
//   level.push_back(p_.level());
//   OFDictData::list levels;
//   levels.push_back(level);
//   castdict["levels"]=levels;
//   sHMDict.subDict("castellatedMeshControls").subDict("refinementRegions")[p_.name()]=castdict;
// }
bool RefinementGeometry::setGeometrySubdict(OFDictData::dict& geodict, std::string& entryTitle) const
{
  entryTitle=geometryfile_.fileName().c_str();
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p_.name;
//   //boost::filesystem::path x; x.f
//   sHMDict.subDict("geometry")[p_.fileName().filename().c_str()]=geodict;
}

void RefinementGeometry::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
  geometryfile_.putIntoConstantTrisurface(ofc, location);
}



defineType(NearSurfaceRefinement);
addToFactoryTable(Feature, NearSurfaceRefinement);
addToStaticFunctionTable(Feature, NearSurfaceRefinement, defaultParameters);


NearSurfaceRefinement::NearSurfaceRefinement(const ParameterSet& ps)
: RefinementRegion(ps)
{
}

bool NearSurfaceRefinement::setGeometrySubdict(OFDictData::dict&, std::string&) const
{
  // do nothing
  return false;
}





defineType(NearTemplatePatchRefinement);
addToFactoryTable(Feature, NearTemplatePatchRefinement);
addToStaticFunctionTable(Feature, NearTemplatePatchRefinement, defaultParameters);

NearTemplatePatchRefinement::NearTemplatePatchRefinement
(
  const ParameterSet& ps
)
: RefinementRegion(ps),
  p_(ps)
{
}

void NearTemplatePatchRefinement::modifyFiles(const OpenFOAMCase& ofc, const path& location) const
{
  boost::filesystem::path to( boost::filesystem::path("constant")/"triSurface"/p_.fileName );

  if (!exists((location/to).parent_path()))
    create_directories((location/to).parent_path());

  ofc.executeCommand(location, "surfaceMeshTriangulate",
    list_of<std::string>
    ("-patches")
    ("("+p_.name+")")
    (to.string())
  );
}


bool NearTemplatePatchRefinement::setGeometrySubdict(OFDictData::dict& geodict, std::string& entryTitle) const
{
  entryTitle=p_.fileName.string();
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p_.name;
}




}

void setStdCastellatedCtrls(OFDictData::dict& castellatedCtrls)
{
  castellatedCtrls["maxLocalCells"]=1000000;
  castellatedCtrls["maxGlobalCells"]=10000000;
  castellatedCtrls["minRefinementCells"]=10;
  castellatedCtrls["maxLoadUnbalance"]=0.1;
  castellatedCtrls["nCellsBetweenLevels"]=8;
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
  layerCtrls["featureAngle"]=60.0;  

  layerCtrls["slipFeatureAngle"]=30.0;  

  layerCtrls["nRelaxIter"]=10;  
  layerCtrls["nSmoothSurfaceNormals"]=1;  
  layerCtrls["nSmoothNormals"]=3;  
  layerCtrls["nSmoothThickness"]=10;  
  layerCtrls["maxFaceThicknessRatio"]=10.0;  
  layerCtrls["maxThicknessToMedialRatio"]=0.75;  
  layerCtrls["minMedianAxisAngle"]=130.0;  
  layerCtrls["nBufferCellsNoExtrude"]=0;  
  layerCtrls["nLayerIter"]=2;  //OCFD
  layerCtrls["maxLayerIter"]=2;  // engys
}

void setStdQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=65.0;
  qualityCtrls["maxBoundarySkewness"]=20.0;
  qualityCtrls["maxInternalSkewness"]=4.0;
  qualityCtrls["maxConcave"]=80.0;  
  qualityCtrls["minFlatness"]=0.01;  
  qualityCtrls["minVol"]=1e-13;  
  qualityCtrls["minArea"]=-1.0;  
  qualityCtrls["minTwist"]=0.02;  
  qualityCtrls["minDeterminant"]=0.001;  
  qualityCtrls["minFaceWeight"]=0.05;  
  qualityCtrls["minVolRatio"]=0.01;  
  qualityCtrls["minTriangleTwist"]=-1.0;  
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  

  qualityCtrls["minTetQuality"]=1e-40;  
}

void setRelaxedQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=85.0;
  qualityCtrls["maxBoundarySkewness"]=20.0;
  qualityCtrls["maxInternalSkewness"]=4.0;
  qualityCtrls["maxConcave"]=180.0; //85.0;  
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

  qualityCtrls["minTetQuality"]= -1; //1e-40;  
}

void setDisabledQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=180.0;
  qualityCtrls["maxBoundarySkewness"]=20.0;
  qualityCtrls["maxInternalSkewness"]=4.0;
  qualityCtrls["maxConcave"]=180.0;  
  qualityCtrls["minFlatness"]=0.002;  
  qualityCtrls["minVol"]=-1e30;  
  qualityCtrls["minArea"]=-1.0;  
  qualityCtrls["minTwist"]=-1;  
  qualityCtrls["minDeterminant"]=1e-6;  
  qualityCtrls["minFaceWeight"]=1e-6;  
  qualityCtrls["minVolRatio"]=1e-6;  
  qualityCtrls["minTriangleTwist"]=-1.0;  
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  

  qualityCtrls["minTetQuality"]=1e-40;  
}


void setNoQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=180.0;
  qualityCtrls["maxBoundarySkewness"]=-1;
  qualityCtrls["maxInternalSkewness"]=-1;
  qualityCtrls["maxConcave"]=180.0;  
  qualityCtrls["minFlatness"]=0.002;  
  qualityCtrls["minVol"]=-1e30;  
  qualityCtrls["minArea"]=-1.0;  
  qualityCtrls["minTwist"]=-1;  
  qualityCtrls["minDeterminant"]=0;  
  qualityCtrls["minFaceWeight"]=0;  
  qualityCtrls["minVolRatio"]=0;  
  qualityCtrls["minTriangleTwist"]=-1.0;  
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  

  qualityCtrls["minTetQuality"]=-1e30;  
}

double computeFinalLayerThickness(double totalLayerThickness, double expRatio, int nlayer)
{
  return totalLayerThickness*pow(expRatio, nlayer-1)*(expRatio-1.0)/(pow(expRatio, nlayer)-1.0);
}





defineType(snappyHexMeshConfiguration);
addToOpenFOAMCaseElementFactoryTable(snappyHexMeshConfiguration);

snappyHexMeshConfiguration::snappyHexMeshConfiguration( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "snappyHexMeshConfiguration"),
  p_(ps)
{
}

void snappyHexMeshConfiguration::addIntoDictionaries(OFdicts& dictionaries) const
{
  using namespace snappyHexMeshFeats;
  
  OFDictData::dict& sHMDict=dictionaries.addDictionaryIfNonexistent("system/snappyHexMeshDict");
  
  const snappyHexMeshConfiguration::Parameters& p = p_;
  
  // setup dict structure
  sHMDict["castellatedMesh"] = p.doCastellatedMesh;
  sHMDict["snap"] = p.doSnap;
  sHMDict["addLayers"] = p.doAddLayers;
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
  
  if (p.PiM.size()>1)
  {
    OFDictData::list PiM;
    BOOST_FOREACH(const snappyHexMeshConfiguration::Parameters::PiM_default_type& pim, p.PiM)
    {
        PiM.push_back(OFDictData::vector3(pim));
    }
    castellatedCtrls["locationInMesh"]=PiM;
  }
  else if (p.PiM.size()==1)
  {
    castellatedCtrls["locationInMesh"]=OFDictData::vector3(p.PiM[0]);
  }
  else
      throw insight::Exception("snappyHexMesh: at least one point in mesh has to be provided!");
  
  setStdLayerCtrls(layerCtrls);
  layerCtrls["relativeSizes"]=p.relativeSizes;
  layerCtrls["finalLayerThickness"]=p.tlayer;
  layerCtrls["expansionRatio"]=p.erlayer;
  layerCtrls["nLayerIter"]=p.nLayerIter;  //OCFD
  layerCtrls["maxLayerIter"]=p.nLayerIter;  // engys
  
  if (p.qualityCtrls==snappyHexMeshConfiguration::Parameters::standard)
  {
    setStdQualityCtrls(qualityCtrls);
  }
  else if (p.qualityCtrls==snappyHexMeshConfiguration::Parameters::relaxed)
  {
    setRelaxedQualityCtrls(qualityCtrls);
  }
  else if (p.qualityCtrls==snappyHexMeshConfiguration::Parameters::disabled)
  {
    setNoQualityCtrls(qualityCtrls);
  }

//   BOOST_FOREACH( const snappyHexMeshFeats::Feature& feat, ops)
  BOOST_FOREACH(const snappyHexMeshConfiguration::Parameters::features_default_type& feat, p.features)
  {
//       feat->modifyFiles(ofc, location);
      feat->addIntoDictionary(sHMDict);
  }
  
}

void snappyHexMeshConfiguration::modifyCaseOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const
{
  BOOST_FOREACH(const snappyHexMeshConfiguration::Parameters::features_default_type& feat, p_.features)
  {
      feat->modifyFiles(cm, location);
//       feat->addIntoDictionary(sHMDict);
  }
}



void snappyHexMesh
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
//   const OFDictData::list& PiM,
//   const boost::ptr_vector<snappyHexMeshFeats::Feature>& ops,
//   snappyHexMeshOpts::Parameters const& p,
  const ParameterSet& ps,
  bool overwrite,
  bool isalreadydecomposed,
  bool keepdecomposedafterfinish
)
{
  using namespace snappyHexMeshFeats;
  
  OFDictData::dictFile sHMDict;
  
  snappyHexMeshConfiguration::Parameters p(ps);
  
  // setup dict structure
  sHMDict["castellatedMesh"] = p.doCastellatedMesh;
  sHMDict["snap"] = p.doSnap;
  sHMDict["addLayers"] = p.doAddLayers;
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
  if (p.PiM.size()>1)
  {
    OFDictData::list PiM;
    BOOST_FOREACH(const snappyHexMeshConfiguration::Parameters::PiM_default_type& pim, p.PiM)
    {
        PiM.push_back(OFDictData::vector3(pim));
    }
    castellatedCtrls["locationInMesh"]=PiM;
  }
  else if (p.PiM.size()==1)
  {
    castellatedCtrls["locationInMesh"]=OFDictData::vector3(p.PiM[0]);
  }
  else
      throw insight::Exception("snappyHexMesh: at least one point in mesh has to be provided!");
  
  setStdLayerCtrls(layerCtrls);
  layerCtrls["relativeSizes"]=p.relativeSizes;
  layerCtrls["finalLayerThickness"]=p.tlayer;
  layerCtrls["expansionRatio"]=p.erlayer;
  layerCtrls["nLayerIter"]=p.nLayerIter;  //OCFD
  layerCtrls["maxLayerIter"]=p.nLayerIter;  // engys
  
  if (p.qualityCtrls==snappyHexMeshConfiguration::Parameters::standard)
  {
    setStdQualityCtrls(qualityCtrls);
  }
  else if (p.qualityCtrls==snappyHexMeshConfiguration::Parameters::relaxed)
  {
    setRelaxedQualityCtrls(qualityCtrls);
  }
  else if (p.qualityCtrls==snappyHexMeshConfiguration::Parameters::disabled)
  {
    setNoQualityCtrls(qualityCtrls);
  }

//   BOOST_FOREACH( const snappyHexMeshFeats::Feature& feat, ops)
  BOOST_FOREACH(const snappyHexMeshConfiguration::Parameters::features_default_type& feat, p.features)
  {
      feat->modifyFiles(ofc, location);
      feat->addIntoDictionary(sHMDict);
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

  if (is_parallel && (!isalreadydecomposed) )
  {
    ofc.executeCommand(location, "decomposePar");
  }
  
  //cm.runSolver(executionPath(), analyzer, solverName, &stopFlag_, np);
  std::vector<std::string> output;
  ofc.executeCommand(location, "snappyHexMesh", opts, &output, np);

  
  // Check fraction of extruded faces on wall patches
  boost::regex re_extrudedfaces("^Extruding ([0-9]+) out of ([0-9]+) faces.*");
  boost::match_results<std::string::const_iterator> what;
  int exfaces=-1, totalfaces=-1;
  BOOST_FOREACH(const std::string& line, output)
  {
    if (boost::regex_match(line, what, re_extrudedfaces))
    {
      //cout<< "\""<<line<<"\""<<what[1]<<", "<<what[2]<<endl;
      exfaces=lexical_cast<int>(what[1]);
      totalfaces=lexical_cast<int>(what[2]);
    }
  }
  if (totalfaces>=0)
  {
    double exfrac=double(exfaces)/double(totalfaces);
    if (exfrac<0.9)
    {
      std::string msg=
      "Prism layer covering is only "+str(format("%g")%(100.*exfrac))+"\% (<90%)!\n"
      "Please reconsider prism layer thickness and tune number of prism layers!";
      
      if (p.stopOnBadPrismLayer)
      {
	throw insight::Exception(msg);
      }
      else
	insight::Warning(msg);
    }
  }
  
  if (is_parallel && (!keepdecomposedafterfinish) )
  {
    ofc.executeCommand(location, "reconstructParMesh", list_of<string>("-constant") );
    ofc.removeProcessorDirectories(location);
  }
  
  
  //ofc.executeCommand(location, "snappyHexMesh", opts);
}


  
}
