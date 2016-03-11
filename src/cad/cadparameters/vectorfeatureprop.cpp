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

#include "cadfeature.h"
#include "vectorfeatureprop.h"

insight::cad::PointFeatureProp::PointFeatureProp
(
  insight::cad::FeaturePtr model, 
  const std::string& name
)
: model_(model),
  name_(name)
{}


arma::mat insight::cad::PointFeatureProp::value() const
{
  return model_->getDatumPoint(name_);
}




insight::cad::VectorFeatureProp::VectorFeatureProp
(
  insight::cad::FeaturePtr model, 
  const std::string& name
)
: model_(model),
  name_(name)
{}


arma::mat insight::cad::VectorFeatureProp::value() const
{
  return model_->getDatumVector(name_);
}

insight::cad::SinglePointCoords::SinglePointCoords(insight::cad::ConstFeatureSetPtr pfs)
: pfs_(pfs)
{}

arma::mat insight::cad::SinglePointCoords::value() const
{
  if (!pfs_->size()==1)
    throw insight::Exception("vertex feature set must not contain more than one point for coordinate extraction!");
  
  FeatureID i=*(pfs_->data().begin());
  return pfs_->model()->vertexLocation(i);
}

insight::cad::CircleEdgeCenterCoords::CircleEdgeCenterCoords(insight::cad::ConstFeatureSetPtr pfs)
: pfs_(pfs)
{}

arma::mat insight::cad::CircleEdgeCenterCoords::value() const
{
  if (!pfs_->size()==1)
    throw insight::Exception("edge feature set must not contain more than one edge for coordinate extraction!");
  
  FeatureID i=*(pfs_->data().begin());
  
  TopLoc_Location tl;
  double c0, c1;
  GeomAdaptor_Curve adapt(BRep_Tool::Curve(pfs_->model()->edge(i), tl, c0, c1));
  if (adapt.GetType()!=GeomAbs_Circle)
    throw insight::Exception("selected edge is not a  circle! (instead is of type "+boost::lexical_cast<std::string>(adapt.GetType())+")");
  
  gp_Circ icyl=adapt.Circle();
  gp_Pnt p0=icyl.Location();
  return vec3(p0.X(), p0.Y(), p0.Z());
}