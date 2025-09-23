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
 */

#ifndef INSIGHT_CAD_VECTORFEATUREPROP_H
#define INSIGHT_CAD_VECTORFEATUREPROP_H

#include "cadtypes.h"
#include "cadparameter.h"

namespace insight 
{
namespace cad 
{

    
    
    
class PointFeatureProp 
: public insight::cad::Vector
{
  FeaturePtr model_;
  std::string name_;

  PointFeatureProp(const PointFeatureProp&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
  CLONEABLE(PointFeatureProp);

  PointFeatureProp(FeaturePtr model, const std::string& name);
  arma::mat value() const override;
};




class VectorFeatureProp 
: public insight::cad::Vector
{
  FeaturePtr model_;
  std::string name_;
  
  VectorFeatureProp(const VectorFeatureProp&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
  CLONEABLE(VectorFeatureProp);

  VectorFeatureProp(FeaturePtr model, const std::string& name);
  arma::mat value() const override;
};




class SinglePointCoords 
: public insight::cad::Vector
{
  ConstFeatureSetPtr pfs_;

    SinglePointCoords(const SinglePointCoords&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(SinglePointCoords);

    SinglePointCoords(ConstFeatureSetPtr pfs);
  arma::mat value() const override;
};




class CircleEdgeCenterCoords 
: public insight::cad::Vector
{
  ConstFeatureSetPtr pfs_;

    CircleEdgeCenterCoords(const CircleEdgeCenterCoords&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(CircleEdgeCenterCoords);

  CircleEdgeCenterCoords(ConstFeatureSetPtr pfs);
  void compute(arma::mat& pc, double& D, arma::mat& ex) const;
  arma::mat value() const override;
};




class DatumPointCoord
: public insight::cad::Vector
{
  ConstDatumPtr pfs_;

    DatumPointCoord(const DatumPointCoord&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(DatumPointCoord);

  DatumPointCoord(ConstDatumPtr pfs);
   arma::mat value() const override;
};




class DatumDir
: public insight::cad::Vector
{
  ConstDatumPtr pfs_;

    DatumDir(const DatumDir&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(DatumDir);

  DatumDir(ConstDatumPtr pfs);
  arma::mat value() const override;
};


class XsecCurveCurve
: public insight::cad::Vector
{
  ConstFeaturePtr c1_, c2_;

    XsecCurveCurve(const XsecCurveCurve&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((c1_, c2_));
#endif
    CLONEABLE(XsecCurveCurve);

  XsecCurveCurve(ConstFeaturePtr c1, ConstFeaturePtr c2);
  arma::mat value() const override;
};



class DatumPlaneNormal
: public insight::cad::Vector
{
  ConstDatumPtr pfs_;

    DatumPlaneNormal(const DatumPlaneNormal&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(DatumPlaneNormal);

  DatumPlaneNormal(ConstDatumPtr pfs);
  arma::mat value() const override;
};



class DatumPlaneX
    : public insight::cad::Vector
{
    ConstDatumPtr pfs_;

    DatumPlaneX(const DatumPlaneX&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(DatumPlaneX);

    DatumPlaneX(ConstDatumPtr pfs);
    arma::mat value() const override;
};



class DatumPlaneY
    : public insight::cad::Vector
{
    ConstDatumPtr pfs_;

    DatumPlaneY(const DatumPlaneY&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((pfs_));
#endif
    CLONEABLE(DatumPlaneY);

    DatumPlaneY(ConstDatumPtr pfs);
    arma::mat value() const override;
};



class BBMin
: public insight::cad::Vector
{
  FeaturePtr model_;

    BBMin(const BBMin&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
    CLONEABLE(BBMin);

  BBMin(FeaturePtr model);
  arma::mat value() const override;
};




class BBMax
: public insight::cad::Vector
{
  FeaturePtr model_;

    BBMax(const BBMax&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
    CLONEABLE(BBMax);

  BBMax(FeaturePtr model);
  arma::mat value() const override;
};




class COG
: public insight::cad::Vector
{
  FeaturePtr model_;

    COG(const COG&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
    CLONEABLE(COG);

  COG(FeaturePtr model);
  arma::mat value() const override;
};




class SurfaceCOG
: public insight::cad::Vector
{
  FeaturePtr model_;

   SurfaceCOG (const SurfaceCOG&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
    CLONEABLE(SurfaceCOG);

  SurfaceCOG(FeaturePtr model);
  arma::mat value() const override;
};




class SurfaceInertiaAxis
: public insight::cad::Vector
{
  FeaturePtr model_;
  int axis_;
  
  SurfaceInertiaAxis(const SurfaceInertiaAxis&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_));
#endif
  CLONEABLE(SurfaceInertiaAxis);

  SurfaceInertiaAxis(FeaturePtr model, int axis);
  arma::mat value() const override;
};



class PointInFeatureCS
    : public insight::cad::Vector
{
    FeaturePtr model_;
    VectorPtr locP_;

    PointInFeatureCS(const PointInFeatureCS&o, TreeCloneMap& tcm);
public:
#ifndef SWIG
    DEPENDS((model_,locP_));
#endif
    CLONEABLE(PointInFeatureCS);

    PointInFeatureCS(FeaturePtr model, VectorPtr locP);
    arma::mat value() const override;
};



}
}

#endif // INSIGHT_CAD_VECTORFEATUREPROP_H
