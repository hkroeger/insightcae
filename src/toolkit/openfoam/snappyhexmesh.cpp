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

#include "openfoam/snappyhexmeshoutputanalyzer.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;

namespace insight
{
  
enum trimmedMesher {sHM, cfM};


// ExternalGeometryFile::ExternalGeometryFile(ParameterSetInput ip)
//     : ps_(&ip.parameterSet()), p_(ip.create<Parameters>())
// {
//     auto fname=p().fileName->originalFilePath().filename().string();
//     insight::assertion(
//         std::isalpha(fname[0]),
//         "filename must not start with a number or special char (got %s)",
//         fname.c_str() );
// //  std::cout<<"added \""<<p_.fileName<<"\""<<std::endl;
// }

// std::string ExternalGeometryFile::fileName() const
// {
//     return p().fileName->fileName().string();
// }


// void ExternalGeometryFile::putIntoConstantTrisurface(const OpenFOAMCase& ofc, const path& location) const
// {
//   boost::filesystem::path from( p().fileName->filePath(location) );
//   boost::filesystem::path to( ExternalGeometryFile::geometryDir(ofc, location)/from.filename() );
  
//   if (!exists(to.parent_path()))
//     create_directories(to.parent_path());

//   ofc.executeCommand(location, "surfaceTransformPoints",
//     {
//      absolute(from).string(),
//      absolute(to).string(),
//      "-scale", OFDictData::to_OF(p().scale),
//      "-translate", OFDictData::to_OF(p().translate),
//      "-rollPitchYaw", OFDictData::to_OF(p().rollPitchYaw)
//     }
//   );
// }


// boost::filesystem::path
// ExternalGeometryFile::geometryDir(
//     const OFEnvironment& /*ofe*/,
//     const boost::filesystem::path& caseDir )
// {
//     return caseDir/"constant"/"triSurface";
// }

// boost::filesystem::path
// ExternalGeometryFile::geometryDir(
//     const OpenFOAMCase &cm, // to be able to check version
//     const boost::filesystem::path &caseDir )
// {
//     return geometryDir(cm.ofe(), caseDir);
// }

  
  
  
namespace snappyHexMeshFeats
{


static boost::filesystem::path
geometryDir(const OFEnvironment& ofe, const boost::filesystem::path& caseDir)
{
    return caseDir/"constant"/"triSurface";
}

static boost::filesystem::path
geometryDir(const OpenFOAMCase& cm, const boost::filesystem::path& caseDir)
{
    return geometryDir(cm.ofe(), caseDir);
}


    
    
defineType(Feature);
defineDynamicClass(Feature);

Feature::Feature(ParameterSetInput ip)
 : p_(ip.forward<Parameters>())
{}

void Feature::modifyFiles
(
  const OpenFOAMCase&,
  const boost::filesystem::path&
) const
{
}

bool Feature::producesPrismLayers() const
{
  return false;
}



defineType(Geometry);
addToFactoryTable(Feature, Geometry);
addToStaticFunctionTable(Feature, Geometry, defaultParameters);


Geometry::Geometry( ParameterSetInput ip )
    : ExternalGeometryFile<Feature>(ip.forward<Parameters>())
{}

void Geometry::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p().name;
    //boost::filesystem::path x; x.f
  std::string fn=p().fileName->fileName().string();
  if (!isalpha(fn[0]))
    fn="\""+fn+"\"";
  sHMDict.subDict("geometry")[fn]=geodict;

  OFDictData::dict castdict;
  OFDictData::list levels;
  levels.push_back(p().minLevel);
  levels.push_back(p().maxLevel);
  castdict["level"]=levels;
  if (p().zoneName!="")
  {
    castdict["faceZone"]=p().zoneName;
    castdict["cellZone"]=p().zoneName;
    castdict["cellZoneInside"]="inside";
  }
  if (p().regionRefinements.size()>0)
  {
    OFDictData::dict rrd;
    for (const Parameters::regionRefinements_default_type& rr: p().regionRefinements)
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
  sHMDict.subDict("castellatedMeshControls").subDict("refinementSurfaces")[p().name]=castdict;

  OFDictData::dict layerdict;
  layerdict["nSurfaceLayers"]=p().nLayers;
  sHMDict.subDict("addLayersControls").subDict("layers")["\""+p().name+".*\""]=layerdict;
  for (const Parameters::regionRefinements_default_type& rr: p().regionRefinements)
  {
   OFDictData::dict layerdict;
   layerdict["nSurfaceLayers"]=0;
   sHMDict.subDict("addLayersControls").subDict("layers")[p().name+"_"+rr.regionname]=layerdict;
  }

}

void Geometry::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
  ExternalGeometryFile::putIntoConstantTrisurface(ofc, location);
}

bool Geometry::producesPrismLayers() const
{
  return p().nLayers>0;
}




defineType(PatchLayers);
addToFactoryTable(Feature, PatchLayers);
addToStaticFunctionTable(Feature, PatchLayers, defaultParameters);

PatchLayers::PatchLayers(ParameterSetInput ip)
    : Feature(ip.forward<Parameters>())
{
}


void PatchLayers::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict layerdict;
  layerdict["nSurfaceLayers"]=p().nLayers;
  sHMDict.subDict("addLayersControls").subDict("layers")[p().name]=layerdict;
}

bool PatchLayers::producesPrismLayers() const
{
  return p().nLayers>0;
}




defineType(ExplicitFeatureCurve);
addToFactoryTable(Feature, ExplicitFeatureCurve);
addToStaticFunctionTable(Feature, ExplicitFeatureCurve, defaultParameters);


ExplicitFeatureCurve::ExplicitFeatureCurve(ParameterSetInput ip)
    : Feature(ip.forward<Parameters>())
{
}

void ExplicitFeatureCurve::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict refdict;
  refdict["file"]=std::string("\"")+p().fileName->fileName().string()+"\"";
  refdict["levels"]=OFDictData::list( {OFDictData::list({p().distance, p().level}) });
  sHMDict.subDict("castellatedMeshControls").getList("features").push_back(refdict);

}

void ExplicitFeatureCurve::modifyFiles(const OpenFOAMCase& ofc, const path& location) const
{
  boost::filesystem::path from(p().fileName->filePath(location));

  if (!exists(from))
  {
    boost::filesystem::path alt_from=from;
    alt_from.replace_extension(".eMesh.gz");
    if (!exists(alt_from))
      throw insight::Exception(
            "feature edge file does not exist: neither %s nor %s",
            from.string().c_str(), alt_from.string().c_str() );
  }

  boost::filesystem::path to(
      snappyHexMeshFeats::geometryDir(ofc, location)
      / from.filename() );
  
  ofc.executeCommand(location, "eMeshTransformPoints",
    {
     absolute(from).string(),
     absolute(to).string(),
     "-scale", OFDictData::to_OF(p().scale),
     "-translate", OFDictData::to_OF(p().translate),
     "-rollPitchYaw", OFDictData::to_OF(p().rollPitchYaw)
    }
  );
}



defineType(RefinementRegion);

RefinementRegion::RefinementRegion(ParameterSetInput ip)
    : Feature(ip.forward<Parameters>())
{
}

void RefinementRegion::addIntoDictionary(OFDictData::dict& sHMDict) const
{
  OFDictData::dict geodict;
  std::string entryTitle=p().name;
  if (setGeometrySubdict(geodict, entryTitle))
    sHMDict.subDict("geometry")[entryTitle]=geodict;

  OFDictData::dict castdict;
  std::string mode;
  if ( p().mode == Parameters::inside )
  {
      mode="inside";
  }
  else if (p().mode==Parameters::outside)
  {
      mode="outside";
  }
  else if (p().mode == Parameters::distance)
  {
      mode="distance";
  }
  castdict["mode"]=mode;
  OFDictData::list level;
  level.push_back(p().dist);
  level.push_back(p().level);
  OFDictData::list levels;
  levels.push_back(level);
  castdict["levels"]=levels;
  sHMDict.subDict("castellatedMeshControls").subDict("refinementRegions")[p().name]=castdict;
}




defineType(RefinementBox);
addToFactoryTable(Feature, RefinementBox);
addToStaticFunctionTable(Feature, RefinementBox, defaultParameters);

RefinementBox::RefinementBox(ParameterSetInput ip)
    : RefinementRegion(ip.forward<Parameters>())
{
}

bool RefinementBox::setGeometrySubdict(OFDictData::dict& d, std::string&) const
{
  d["type"]="searchableBox";
  d["min"]=OFDictData::to_OF(p().min);
  d["max"]=OFDictData::to_OF(p().max);
  return true;
}





defineType(RefinementCylinder);
addToFactoryTable(Feature, RefinementCylinder);
addToStaticFunctionTable(Feature, RefinementCylinder, defaultParameters);

RefinementCylinder::RefinementCylinder(ParameterSetInput ip)
    : RefinementRegion(ip.forward<Parameters>())
{}

bool RefinementCylinder::setGeometrySubdict(OFDictData::dict& d, std::string&) const
{
  d["type"]="searchableCylinder";
  d["point1"]=OFDictData::to_OF(p().point1);
  d["point2"]=OFDictData::to_OF(p().point2);
  d["radius"]=p().radius;
  return true;
}





defineType(RefinementSphere);
addToFactoryTable(Feature, RefinementSphere);
addToStaticFunctionTable(Feature, RefinementSphere, defaultParameters);

RefinementSphere::RefinementSphere(ParameterSetInput ip)
    : RefinementRegion(ip.forward<Parameters>())
{
}

bool RefinementSphere::setGeometrySubdict(OFDictData::dict& d, std::string&) const
{
  d["type"]="searchableSphere";
  d["centre"]=OFDictData::to_OF(p().center);
  d["radius"]=p().radius;
  return true;
}





defineType(RefinementGeometry);
addToFactoryTable(Feature, RefinementGeometry);
addToStaticFunctionTable(Feature, RefinementGeometry, defaultParameters);

RefinementGeometry::RefinementGeometry(ParameterSetInput ip)
: ExternalGeometryFile<RefinementRegion>(ip.forward<Parameters>())
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
  entryTitle=fileName().c_str();
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p().name;
//   //boost::filesystem::path x; x.f
//   sHMDict.subDict("geometry")[p_.fileName().filename().c_str()]=geodict;
  return true;
}

void RefinementGeometry::modifyFiles(const OpenFOAMCase& ofc, 
	      const boost::filesystem::path& location) const
{
  putIntoConstantTrisurface(ofc, location);
}



defineType(NearSurfaceRefinement);
addToFactoryTable(Feature, NearSurfaceRefinement);
addToStaticFunctionTable(Feature, NearSurfaceRefinement, defaultParameters);


NearSurfaceRefinement::NearSurfaceRefinement(ParameterSetInput ip)
    : RefinementRegion(ip.forward<Parameters>())
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

NearTemplatePatchRefinement::NearTemplatePatchRefinement(
    ParameterSetInput ip )
    : RefinementRegion(ip.forward<Parameters>())
{}

void NearTemplatePatchRefinement::modifyFiles(const OpenFOAMCase& ofc, const path& location) const
{
  boost::filesystem::path to( boost::filesystem::path("constant")/"triSurface"/p().fileName->fileName() );

  if (!exists((location/to).parent_path()))
    create_directories((location/to).parent_path());

  ofc.executeCommand(location,
    ofc.OFversion() <=600 ? "surfaceMeshTriangulate" : "surfaceMeshExtract",
    { "-patches", "("+p().name+")", to.string() }
  );
}


bool NearTemplatePatchRefinement::setGeometrySubdict(OFDictData::dict& geodict, std::string& entryTitle) const
{
  entryTitle=p().fileName->fileName().string();
  geodict["type"]="triSurfaceMesh";
  geodict["name"]=p().name;
  return true;
}




}

void setStdCastellatedCtrls(OFDictData::dict& castellatedCtrls)
{
  castellatedCtrls["maxLocalCells"]  = 10000000;
  castellatedCtrls["maxGlobalCells"] = 100000000;
  castellatedCtrls["minRefinementCells"] = 10;
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
  snapCtrls["nRelaxIter"]=15;  

  snapCtrls["nFeatureSnapIter"]=15;  
  snapCtrls["implicitFeatureSnap"]=true;
  snapCtrls["explicitFeatureSnap"]=false;
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
  qualityCtrls["minArea"]=1e-30;
  qualityCtrls["minTwist"]=0.02;  
  qualityCtrls["minDeterminant"]=0.001;  
  qualityCtrls["minFaceWeight"]=0.05;  
  qualityCtrls["minVolRatio"]=0.01;  
  qualityCtrls["minTriangleTwist"]=0.01;
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  

  qualityCtrls["minTetQuality"]=1e-15;
}

void setRelaxedQualityCtrls(OFDictData::dict& qualityCtrls)
{
  qualityCtrls["maxNonOrtho"]=75.0;
  qualityCtrls["maxBoundarySkewness"]=20.0;
  qualityCtrls["maxInternalSkewness"]=4.0;
  qualityCtrls["maxConcave"]=85.0;
  qualityCtrls["minFlatness"]=0.002;  
  qualityCtrls["minVol"]=1e-18;  
  qualityCtrls["minArea"]=-1.0;  
  qualityCtrls["minTwist"]=0.001;  
  qualityCtrls["minDeterminant"]=0.00001;  
  qualityCtrls["minFaceWeight"]=0.01;  
  qualityCtrls["minVolRatio"]=0.0005;  
  qualityCtrls["minTriangleTwist"]=-1.0;  
  qualityCtrls["nSmoothScale"]=4;  
  qualityCtrls["errorReduction"]=0.75;  

  qualityCtrls["minTetQuality"]= 1e-40;  
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

  qualityCtrls["minTetQuality"]=-1;  
}

double computeFinalLayerThickness(double totalLayerThickness, double expRatio, int nlayer)
{
  return totalLayerThickness*pow(expRatio, nlayer-1)*(expRatio-1.0)/(pow(expRatio, nlayer)-1.0);
}


RefinementLevel::RefinementLevel(
    const boost::variant<L_delta,L_level,level_delta>& input)
{
    if (const auto * Ld = boost::get<L_delta>(&input))
    {
        L=Ld->L;
        delta=Ld->delta;
        insight::assertion(
            delta>SMALL,
            "refinement level target mesh size must be larger than zero");
        insight::assertion(
            L>SMALL,
            "refinement level target length must be larger than zero");
        double dlevel=log(L/delta)/log(2.);
        level=std::max<int>(0, std::ceil(dlevel));
    }
    else if (const auto * Ll = boost::get<L_level>(&input))
    {
        L=Ll->L;
        level=Ll->level;
        delta=std::max(0., L/pow(2., level));
    }
    else if (const auto * ld = boost::get<level_delta>(&input))
    {
        level=ld->level;
        delta=ld->delta;
        L=std::max(0., delta*pow(2,level));
    }
    else
        throw insight::Exception("unhandled input");
}




defineType(snappyHexMeshConfiguration);
addToOpenFOAMCaseElementFactoryTable(snappyHexMeshConfiguration);

snappyHexMeshConfiguration::snappyHexMeshConfiguration( OpenFOAMCase& c, ParameterSetInput ip )
    : OpenFOAMCaseElement(c, /*"snappyHexMeshConfiguration", */ip.forward<Parameters>())
{
}

void snappyHexMeshConfiguration::addIntoDictionaries(OFdicts& dictionaries) const
{
  using namespace snappyHexMeshFeats;
  
  OFDictData::dict& sHMDict=dictionaries.lookupDict("system/snappyHexMeshDict");
  
  // setup dict structure
  sHMDict["castellatedMesh"] = p().doCastellatedMesh;
  sHMDict["snap"] = p().doSnap;
  sHMDict["addLayers"] = p().doAddLayers;
  sHMDict["debug"] = 0;
  sHMDict["mergeTolerance"] = 1e-6;
  OFDictData::dict& geomCtrls=sHMDict.subDict("geometry");
  OFDictData::dict& castellatedCtrls = sHMDict.subDict("castellatedMeshControls");
  castellatedCtrls.getList("features");
  castellatedCtrls.subDict("refinementSurfaces");
  castellatedCtrls.subDict("refinementRegions");
  OFDictData::dict& snapCtrls=sHMDict.subDict("snapControls");
  OFDictData::dict& layerCtrls=sHMDict.subDict("addLayersControls");
  layerCtrls.subDict("layers");
  OFDictData::dict& qualityCtrls=sHMDict.subDict("meshQualityControls");

  //  populate with defaults
  setStdSnapCtrls(snapCtrls);
  snapCtrls["implicitFeatureSnap"]=p().doImplicitFeatureSnap;
  snapCtrls["explicitFeatureSnap"]=p().doExplicitFeatureSnap;

  setStdCastellatedCtrls(castellatedCtrls);
  castellatedCtrls["allowFreeStandingZoneFaces"]=p().allowFreeStandingZoneFaces;
  
  if (p().PiM.size()>1)
  {
    OFDictData::list PiM;
    int i=1;
    for (const snappyHexMeshConfiguration::Parameters::PiM_default_type& pim: p().PiM)
    {
      if (OFversion()>=600)
      {
        PiM.push_back(OFDictData::list{ OFDictData::vector3(pim), str(format("zone%d")%(i++)) });
      }
      else
      {
        PiM.push_back(OFDictData::vector3(pim));
      }
    }
    castellatedCtrls["locationsInMesh"]=PiM;
  }
  else if (p().PiM.size()==1)
  {
    castellatedCtrls["locationInMesh"]=OFDictData::vector3(p().PiM[0]);
  }
  else
      throw insight::Exception("snappyHexMesh: at least one point in mesh has to be provided!");
  
  setStdLayerCtrls(layerCtrls);
  layerCtrls["relativeSizes"]=p().relativeSizes;
  layerCtrls["finalLayerThickness"]=p().tlayer;
  layerCtrls["expansionRatio"]=p().erlayer;
  layerCtrls["nLayerIter"]=p().nLayerIter;  //OCFD
  layerCtrls["maxLayerIter"]=p().nLayerIter;  // engys
  
  if (p().qualityCtrls==snappyHexMeshConfiguration::Parameters::standard)
  {
    setStdQualityCtrls(qualityCtrls);
  }
  else if (p().qualityCtrls==snappyHexMeshConfiguration::Parameters::relaxed)
  {
    setRelaxedQualityCtrls(qualityCtrls);
  }
  else if (p().qualityCtrls==snappyHexMeshConfiguration::Parameters::disabled)
  {
    setNoQualityCtrls(qualityCtrls);
  }

  for (const snappyHexMeshConfiguration::Parameters::features_default_type& feat: p().features)
  {
      feat->addIntoDictionary(sHMDict);
  }
  
}

void snappyHexMeshConfiguration::modifyCaseOnDisk (
    const OpenFOAMCase& cm, const boost::filesystem::path& location ) const
{
  for (auto& feat: p().features)
  {
      feat->modifyFiles(cm, location);
  }
}


void reconstructParMesh
(
  const OpenFOAMCase& ofc,
  const boost::filesystem::path& location
)
{
    int np=readDecomposeParDict(location);
    bool is_parallel = (np>1);


    if (is_parallel)
    {
      ofc.executeCommand(location, "reconstructParMesh", {"-constant"} );
      ofc.removeProcessorDirectories(location);
    }
}

void snappyHexMesh
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location,
//   const OFDictData::list& PiM,
//   const boost::ptr_vector<snappyHexMeshFeats::Feature>& ops,
//   snappyHexMeshOpts::Parameters const& p,
  const snappyHexMeshConfiguration::Parameters& p,
  bool overwrite,
  bool isalreadydecomposed,
  bool keepdecomposedafterfinish,
  ProgressDisplayer* progress,
  std::function<void(OFDictData::dict&)> sHMDictModifier
)
{
  using namespace snappyHexMeshFeats;
  
  OFDictData::dictFile sHMDict;
  
  // setup dict structure
  sHMDict["castellatedMesh"] = p.doCastellatedMesh;
  sHMDict["snap"] = p.doSnap;
  sHMDict["addLayers"] = p.doAddLayers;
  sHMDict["debug"] = 0;
  sHMDict["mergeTolerance"] = 1e-6;
  OFDictData::dict& geomCtrls=sHMDict.subDict("geometry");
  OFDictData::dict& castellatedCtrls = sHMDict.subDict("castellatedMeshControls");
  castellatedCtrls.getList("features");
  castellatedCtrls.subDict("refinementSurfaces");
  castellatedCtrls.subDict("refinementRegions");
  OFDictData::dict& snapCtrls=sHMDict.subDict("snapControls");
  OFDictData::dict& layerCtrls=sHMDict.subDict("addLayersControls");
  layerCtrls.subDict("layers");
  OFDictData::dict& qualityCtrls=sHMDict.subDict("meshQualityControls");

  //  populate with defaults
  setStdSnapCtrls(snapCtrls);

  snapCtrls["nSmoothPatch"]=p.nSmoothPatch;
  snapCtrls["implicitFeatureSnap"]=p.doImplicitFeatureSnap;
  snapCtrls["explicitFeatureSnap"]=p.doExplicitFeatureSnap;

  setStdCastellatedCtrls(castellatedCtrls);
  castellatedCtrls["allowFreeStandingZoneFaces"]=p.allowFreeStandingZoneFaces;

  if (p.PiM.size()>1)
  {
    OFDictData::list PiM;
    int i=0;
    for (const snappyHexMeshConfiguration::Parameters::PiM_default_type& pim: p.PiM)
    {
      if (ofc.OFversion()>=600)
      {
        std::string zoneName="zone%d";

        if (p.PiMZoneNames.size()>=size_t(i+1))
        {
          zoneName=p.PiMZoneNames[i];
        }

        if (zoneName.find("%d")!=std::string::npos)
        {
          zoneName=str(format(zoneName)%(i+1));
        }

        PiM.push_back(OFDictData::list{ OFDictData::vector3(pim), zoneName });
      }
      else
      {
        PiM.push_back(OFDictData::vector3(pim));
      }

      ++i;
    }
    castellatedCtrls["locationsInMesh"]=PiM;
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

  for (const snappyHexMeshConfiguration::Parameters::features_default_type& feat: p.features)
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

  if (sHMDictModifier) sHMDictModifier(sHMDict);
  
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
//  std::vector<std::string> output;

  snappyHexMeshOutputAnalyzer saa(progress);
//  ofc.executeCommand(location, "snappyHexMesh", opts, &output, np);
  ofc.runSolver(location, saa, "snappyHexMesh", np, opts);

  
  // Check fraction of extruded faces on wall patches
//  boost::regex re_extrudedfaces("^Extruding ([0-9]+) out of ([0-9]+) faces.*");
//  boost::match_results<std::string::const_iterator> what;
//  int exfaces=-1, totalfaces=-1;
//  for (const std::string& line: output)
//  {
//    if (boost::regex_match(line, what, re_extrudedfaces))
//    {
//      //cout<< "\""<<line<<"\""<<what[1]<<", "<<what[2]<<endl;
//      exfaces=lexical_cast<int>(what[1]);
//      totalfaces=lexical_cast<int>(what[2]);
//    }
//  }
  bool anyExtrusionRequested=false;
  for (const auto& f: p.features)
  {
    anyExtrusionRequested = anyExtrusionRequested || f->producesPrismLayers();
  }
  if ( (saa.totalFaces()>=0) && anyExtrusionRequested )
  {
//    double exfrac=double(exfaces)/double(totalfaces);
    if (saa.extrudedFraction()<0.9)
    {
      std::string msg=
      "Prism layer covering is only "+str(format("%g")%(100.*saa.extrudedFraction()))+"\% (<90%)!\n"
      "Please reconsider prism layer thickness and tune number of prism layers!";
      
      if (p.stopOnBadPrismLayer)
      {
	throw insight::Exception(msg);
      }
      else
      {
	insight::Warning(msg);
      }
    }
  }
  
//  if (is_parallel && (!keepdecomposedafterfinish) )
//  {
//    ofc.executeCommand(location, "reconstructParMesh", list_of<string>("-constant") );
//    ofc.removeProcessorDirectories(location);
//  }

  if (!keepdecomposedafterfinish)
  {
    reconstructParMesh(ofc, location);
  }
  
  
  //ofc.executeCommand(location, "snappyHexMesh", opts);
}


  
}
