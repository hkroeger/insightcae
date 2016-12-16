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

#ifndef INSIGHT_SNAPPYHEXMESH_H
#define INSIGHT_SNAPPYHEXMESH_H

#include <string>
#include <vector>

#include "base/boost_include.h"

#include "openfoam/openfoamcase.h"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight {
  
    
    
    
class ExternalGeometryFile
{
public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//       ( fileName, boost::filesystem::path, "" )
//       ( scale, arma::mat, vec3(1,1,1) )
//       ( translate, arma::mat, vec3(0,0,0) )
//       ( rollPitchYaw, arma::mat, vec3(0,0,0) )
//   )
#include "snappyhexmesh__ExternalGeometryFile__Parameters.h"
/*
PARAMETERSET>>> ExternalGeometryFile Parameters

fileName = path "" "Path to geometry file (STL format)"
scale = vector (1 1 1) "Geometry scaling factor for each spatial direction"
translate = vector (0 0 0) "Translation vector"
rollPitchYaw = vector (0 0 0) "Euler angles around X, Y and Z axis respectively"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  ExternalGeometryFile( const ParameterSet& ps = Parameters::makeDefault() );
  
  virtual void putIntoConstantTrisurface
  (
    const OpenFOAMCase& ofc,
    const boost::filesystem::path& location
  ) const;
};




namespace snappyHexMeshFeats
{

class Feature;

typedef boost::shared_ptr<Feature> FeaturePtr;
    



class Feature
{
public:
  declareType ( "Feature" );
  declareDynamicClass ( Feature );

  virtual void addIntoDictionary ( OFDictData::dict& sHMDict ) const =0;
  virtual void modifyFiles ( const OpenFOAMCase& ofc,
                             const boost::filesystem::path& location ) const;
};




class Geometry
: public Feature,
  public ExternalGeometryFile
{
public:
//   typedef boost::tuple<std::string,int,int> RegionRefinement;
//   typedef std::vector<RegionRefinement> RegionRefinementList;
  
//   CPPX_DEFINE_OPTIONCLASS(Parameters, ExternalGeometryFile::Parameters,
//       ( name, std::string, "" )
//       ( minLevel, int, 0 )
//       ( maxLevel, int, 4 )
//       ( nLayers, int, 2 )
//       ( regionRefinements, RegionRefinementList, RegionRefinementList() )
//       ( zoneName, std::string, "" )
//   )
#include "snappyhexmesh__Geometry__Parameters.h"
/*
PARAMETERSET>>> Geometry Parameters
inherits insight::ExternalGeometryFile::Parameters

name = string "" "Name of the geometry feature"
minLevel = int 0 "Minimum refinement level"
maxLevel = int 4 "Maximum refinement level"
nLayers = int 2 "Number of prism layers"
zoneName = string "" "Zone name"
regionRefinements = array [ set {
 regionname = string "" "Name of geometry region"
 minLevel = int 0 "Minimum refinement level"
 maxLevel = int 0 "Maximum refinement level"
} ]*0 "Refinement regions"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("Geometry");
  Geometry(const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }
  
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
  virtual void modifyFiles(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location) const;
};




class PatchLayers
: public Feature
{
public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//       ( name, std::string, "" )
//       ( nLayers, int, 2 )
//   )
#include "snappyhexmesh__PatchLayers__Parameters.h"
/*
PARAMETERSET>>> PatchLayers Parameters

name = string "" "Name of the patch"
nLayers = int 2 "Number of layers"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("PatchLayers");
  PatchLayers(const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }
  
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
};




class ExplicitFeatureCurve
: public Feature
{
public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//       ( fileName, boost::filesystem::path, "" )
//       ( level, int, 4 )
//   )
#include "snappyhexmesh__ExplicitFeatureCurve__Parameters.h"
/*
PARAMETERSET>>> ExplicitFeatureCurve Parameters

fileName = path "" "Filename of the feature curve"
level = int 4 "Refinement level at curve"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("ExplicitFeatureCurve");
  ExplicitFeatureCurve(const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }
  
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
  virtual void modifyFiles(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location) const;
};




class RefinementRegion
  : public Feature
{
public:
#include "snappyhexmesh__RefinementRegion__Parameters.h"
/*
PARAMETERSET>>> RefinementRegion Parameters

name = string "" "Region name"
dist = double 1e15 "Maximum distance for refinement"
mode = selection ( inside outside distance ) inside "Refinement mode"
level = int 1 "Refinement level"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType ( "RefinementRegion" );
  RefinementRegion ( const ParameterSet& ps = Parameters::makeDefault() );


  /**
   * create entry into geometry subdict.
   * supply handle to title, since it is not always equal to name and there might be need to change it
   */
  virtual bool setGeometrySubdict ( OFDictData::dict& d, std::string& entryTitle ) const =0;
  virtual void addIntoDictionary ( OFDictData::dict& sHMDict ) const;
};




class RefinementBox
  : public RefinementRegion
{
public:
#include "snappyhexmesh__RefinementBox__Parameters.h"
/*
PARAMETERSET>>> RefinementBox Parameters
inherits insight::snappyHexMeshFeats::RefinementRegion::Parameters

min = vector (0 0 0) "Minimum corner of refinement box"
max = vector (1 1 1) "Maximum corner of refinement box"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType ( "RefinementBox" );
  RefinementBox ( const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters()
  {
    return Parameters::makeDefault();
  }
  virtual ParameterSet getParameters() const
  {
    return p_;
  }

  virtual bool setGeometrySubdict ( OFDictData::dict& d, std::string& entryTitle ) const;
};





class RefinementGeometry
: public RefinementRegion
{
public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, RefinementRegion::Parameters,
//       ( fileName, boost::filesystem::path, "" )
//   )
#include "snappyhexmesh__RefinementGeometry__Parameters.h"
/*
PARAMETERSET>>> RefinementGeometry Parameters
inherits insight::snappyHexMeshFeats::RefinementRegion::Parameters

fileName = path "" "Path to geometry file (STL format)"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
    declareType("RefinementGeometry");
  RefinementGeometry( const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }

  virtual bool setGeometrySubdict(OFDictData::dict& d, std::string& entryTitle) const;
//   virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
};




class NearSurfaceRefinement
: public RefinementRegion
{
public:
    declareType("NearSurfaceRefinement");
    NearSurfaceRefinement( const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }

  virtual bool setGeometrySubdict(OFDictData::dict& d, std::string& entryTitle) const;
};




class NearTemplatePatchRefinement
  : public RefinementRegion
{
// public:
//   CPPX_DEFINE_OPTIONCLASS(Parameters, RefinementRegion::Parameters,
//       ( fileName, std::string, "" )
//   )
public:
#include "snappyhexmesh__NearTemplatePatchRefinement__Parameters.h"
/*
PARAMETERSET>>> NearTemplatePatchRefinement Parameters
inherits insight::snappyHexMeshFeats::RefinementRegion::Parameters

fileName = path "" "Path to geometry file (STL format)"

<<<PARAMETERSET
*/

protected:
  Parameters p_;

public:
  declareType ( "NearTemplatePatchRefinement" );
  NearTemplatePatchRefinement ( const ParameterSet& ps = Parameters::makeDefault() );
  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }

  virtual void modifyFiles ( const OpenFOAMCase& ofc,
                             const boost::filesystem::path& location ) const;
  virtual bool setGeometrySubdict ( OFDictData::dict& d, std::string& entryTitle ) const;
};

}




class snappyHexMeshConfiguration
    : public OpenFOAMCaseElement
{
public:
#include "snappyhexmesh__snappyHexMeshConfiguration__Parameters.h"
/*
PARAMETERSET>>> snappyHexMeshConfiguration Parameters

doCastellatedMesh = bool true "Enable castellated meshing step"
doSnap = bool true "Enable snapping step"
doAddLayers = bool true "Enable layer addition step"
tlayer= double 0.5 "Layer thickness value"
erlayer = double 1.3 "Expansion ratio of layers"
relativeSizes = bool true "Whether tlayer specifies relative thickness (absolute thickness if set to false)"
nLayerIter= int 10 "Maximum number of layer iterations"
stopOnBadPrismLayer = bool false "Whether to stop of too few layers get extruded"
qualityCtrls = selection ( standard relaxed disabled ) relaxed "Select quality requirements"
PiM = array [ vector (0 0 0) "point inside mesh" ]*0 "One or more points inside meshing domain"
features = array [ dynamicclassconfig "insight::snappyHexMeshFeats::Feature" default "Geometry" "SnappyHexMesh feature" ]*0 "Mesh generation features"
    
<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "snappyHexMeshConfiguration" );
    snappyHexMeshConfiguration ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    virtual void modifyCaseOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};




// namespace snappyHexMeshOpts
// {
//   typedef boost::shared_ptr<OFDictData::dict> DictPtr;
//   
//   CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
//     (doCastellatedMesh, bool, true)
//     (doSnap, bool, true)
//     (doAddLayers, bool, true)
//     (tlayer, double, 0.5)
//     (erlayer, double, 1.3)
//     (relativeSizes, bool, true)
//     (nLayerIter, int, 10 )
//     (stopOnBadPrismLayer, bool, false)
//     (qualityCtrls, DictPtr, DictPtr() )
//   )
// };

void setStdCastellatedCtrls(OFDictData::dict& castellatedCtrls);
void setStdSnapCtrls(OFDictData::dict& snapCtrls);
void setStdLayerCtrls(OFDictData::dict& layerCtrls);
void setStdQualityCtrls(OFDictData::dict& qualityCtrls);
void setRelaxedQualityCtrls(OFDictData::dict& qualityCtrls);
void setDisabledQualityCtrls(OFDictData::dict& qualityCtrls);
void setNoQualityCtrls(OFDictData::dict& qualityCtrls);

double computeFinalLayerThickness(double totalLayerThickness, double expRatio, int nlayer);

void snappyHexMesh
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location, 
//   const OFDictData::list& PiM,
//   const std::vector<snappyHexMeshFeats::FeaturePtr>& ops,
//   snappyHexMeshOpts::Parameters const& p = snappyHexMeshOpts::Parameters(),
  const ParameterSet &ps = snappyHexMeshConfiguration::Parameters::makeDefault(),
  bool overwrite=true,
  bool isalreadydecomposed=false,
  bool keepdecomposedafterfinish=false
);



}

#endif // INSIGHT_SNAPPYHEXMESH_H
