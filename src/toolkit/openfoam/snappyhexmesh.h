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
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( fileName, boost::filesystem::path, "" )
      ( scale, arma::mat, vec3(1,1,1) )
      ( translate, arma::mat, vec3(0,0,0) )
      ( rollPitchYaw, arma::mat, vec3(0,0,0) )
  )

protected:
  Parameters p_;

public:
  ExternalGeometryFile( const Parameters& p = Parameters() );
  
  virtual void putIntoConstantTrisurface
  (
    const OpenFOAMCase& ofc,
    const boost::filesystem::path& location
  ) const;
};

namespace snappyHexMeshFeats
{
  
class Feature
{
public:
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const =0;
  virtual void modifyFiles(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location) const;
  virtual Feature* clone() const =0;
};

inline Feature* new_clone(const Feature& op)
{
  return op.clone();
}


class Geometry
: public Feature,
  public ExternalGeometryFile
{
public:
  typedef boost::tuple<std::string,int,int> RegionRefinement;
  typedef std::vector<RegionRefinement> RegionRefinementList;
  
  CPPX_DEFINE_OPTIONCLASS(Parameters, ExternalGeometryFile::Parameters,
      ( name, std::string, "" )
      ( minLevel, int, 0 )
      ( maxLevel, int, 4 )
      ( nLayers, int, 2 )
      ( regionRefinements, RegionRefinementList, RegionRefinementList() )
      ( zoneName, std::string, "" )
  )

protected:
  Parameters p_;

public:
  Geometry(Parameters const& p = Parameters() );
  
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
  virtual void modifyFiles(const OpenFOAMCase& ofc, 
		  const boost::filesystem::path& location) const;
  
  Feature* clone() const;
};


class PatchLayers
: public Feature
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( name, std::string, "" )
      ( nLayers, int, 2 )
  )

protected:
  Parameters p_;

public:
  PatchLayers(Parameters const& p = Parameters() );
  
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
  
  Feature* clone() const;
};


class ExplicitFeatureCurve
: public Feature
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( fileName, boost::filesystem::path, "" )
      ( level, int, 4 )
  )

protected:
  Parameters p_;

public:
  ExplicitFeatureCurve(Parameters const& p = Parameters() );
  
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
//   virtual void modifyFiles(const OpenFOAMCase& ofc, 
// 		  const boost::filesystem::path& location) const;
  
  Feature* clone() const;
};

class RefinementRegion
: public Feature
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
      ( name, std::string, "" )
      ( distance, double, 1e15 )
      ( mode, std::string, "inside" )
      ( level, int, 1 )
  )

protected:
  Parameters p_;

public:
  RefinementRegion(Parameters const& p = Parameters() );
  
  virtual bool setGeometrySubdict(OFDictData::dict& d) const =0;
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
};

class RefinementBox
: public RefinementRegion
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, RefinementRegion::Parameters,
      ( min, arma::mat, vec3(0,0,0) )
      ( max, arma::mat, vec3(0,0,0) )
  )

protected:
  Parameters p_;

public:
  RefinementBox(Parameters const& p = Parameters() );
  virtual bool setGeometrySubdict(OFDictData::dict& d) const;
  Feature* clone() const;
};


class RefinementGeometry
: public RefinementRegion
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, RefinementRegion::Parameters,
      ( fileName, boost::filesystem::path, "" )
  )

protected:
  Parameters p_;

public:
  RefinementGeometry(Parameters const& p = Parameters() );
  virtual bool setGeometrySubdict(OFDictData::dict& d) const;
  virtual void addIntoDictionary(OFDictData::dict& sHMDict) const;
  Feature* clone() const;
};

class NearSurfaceRefinement
: public RefinementRegion
{
public:
  NearSurfaceRefinement(Parameters const& p = Parameters() );
  virtual bool setGeometrySubdict(OFDictData::dict& d) const;
  Feature* clone() const;
};

}



namespace snappyHexMeshOpts
{
  typedef boost::shared_ptr<OFDictData::dict> DictPtr;
  
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (doCastellatedMesh, bool, true)
    (doSnap, bool, true)
    (doAddLayers, bool, true)
    (tlayer, double, 0.5)
    (erlayer, double, 1.3)
    (relativeSizes, bool, true)
    (nLayerIter, int, 10 )
    (stopOnBadPrismLayer, bool, false)
    (qualityCtrls, DictPtr, DictPtr() )
  )
};

void setStdCastellatedCtrls(OFDictData::dict& castellatedCtrls);
void setStdSnapCtrls(OFDictData::dict& snapCtrls);
void setStdLayerCtrls(OFDictData::dict& layerCtrls);
void setStdQualityCtrls(OFDictData::dict& qualityCtrls);
void setRelaxedQualityCtrls(OFDictData::dict& qualityCtrls);
void setDisabledQualityCtrls(OFDictData::dict& qualityCtrls);

double computeFinalLayerThickness(double totalLayerThickness, double expRatio, int nlayer);

void snappyHexMesh
(
  const OpenFOAMCase& ofc, 
  const boost::filesystem::path& location, 
  const OFDictData::list& PiM,
  const boost::ptr_vector<snappyHexMeshFeats::Feature>& ops,
  snappyHexMeshOpts::Parameters const& p = snappyHexMeshOpts::Parameters(),
  bool overwrite=true
);



}

#endif // INSIGHT_SNAPPYHEXMESH_H
