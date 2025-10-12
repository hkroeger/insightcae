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
#include "scalarfeatureprop.h"
#include "cadfeature.h"
#include "featureset.h"
#include "datum.h"
#include "geotest.h"


insight::cad::ScalarFeatureProp::ScalarFeatureProp(const ScalarFeatureProp &o, TreeCloneMap &tcm)
    :CL(model_), name_(o.name_)
{}

insight::cad::ScalarFeatureProp::ScalarFeatureProp
(
  insight::cad::FeaturePtr model, 
  const std::string& name
)
: model_(model),
  name_(name)
{}

size_t insight::cad::ScalarFeatureProp::calcHash() const
{
    ParameterListHash h;
    h+=*model_;
    h+=name_;
    return h.getHash();
}

double insight::cad::ScalarFeatureProp::calcValue() const
{
  return model_->getDatumScalar(name_);
}


insight::cad::FeatureVolume::FeatureVolume(const FeatureVolume &o, TreeCloneMap &tcm)
    : CL(model_)
{}

insight::cad::FeatureVolume::FeatureVolume(insight::cad::FeaturePtr model)
: model_(model)
{}


size_t insight::cad::FeatureVolume::calcHash() const
{
    ParameterListHash h;
    h+=*model_;
    return h.getHash();
}

double insight::cad::FeatureVolume::calcValue() const
{
  return model_->modelVolume();
}


insight::cad::FeatureSurfaceArea::FeatureSurfaceArea(const FeatureSurfaceArea &o, TreeCloneMap &tcm)
    : CL(model_)
{}

insight::cad::FeatureSurfaceArea::FeatureSurfaceArea(insight::cad::FeaturePtr model)
    : model_(model)
{}

size_t insight::cad::FeatureSurfaceArea::calcHash() const
{
    ParameterListHash h;
    h+=*model_;
    return h.getHash();
}

double insight::cad::FeatureSurfaceArea::calcValue() const
{
    return model_->modelSurfaceArea();
}



insight::cad::CumulativeEdgeLength::CumulativeEdgeLength(const CumulativeEdgeLength &o, TreeCloneMap &tcm)
    : CL(model_)
{}

insight::cad::CumulativeEdgeLength::CumulativeEdgeLength(insight::cad::FeaturePtr model)
: model_(model)
{}

size_t insight::cad::CumulativeEdgeLength::calcHash() const
{
    ParameterListHash h;
    h+=*model_;
    return h.getHash();
}

double insight::cad::CumulativeEdgeLength::calcValue() const
{
    double L=0;
    FeatureSetData ae = model_->allEdgesSet();
    for (const FeatureID& i: ae)
    {
        L+=edgeLength(model_->edge(i));
    }
    return L;
}



insight::cad::CircleDiameter::CircleDiameter(const CircleDiameter &o, TreeCloneMap &tcm)
    : CL(pfs_)
{}

insight::cad::CircleDiameter::CircleDiameter(insight::cad::ConstFeatureSetPtr pfs)
: pfs_(pfs)
{}



size_t insight::cad::CircleDiameter::calcHash() const
{
    ParameterListHash h;
    h+=*pfs_;
    return h.getHash();
}

double insight::cad::CircleDiameter::calcValue() const
{
  if (!(pfs_->size()==1))
    throw insight::Exception("edge feature set must not contain more than one edge for diameter extraction!");

  FeatureID i=*(pfs_->data().begin());

  double c0, c1;
  Handle_Geom_Curve curve(BRep_Tool::Curve(pfs_->model()->edge(i), c0, c1));
  GeomAdaptor_Curve adapt(curve);

  double D;
  if (adapt.GetType()==GeomAbs_Circle)
  {
    gp_Circ icyl=adapt.Circle();
    D=icyl.Radius()*2.;
  }
  else if (adapt.GetType()==GeomAbs_BSplineCurve)
  {
    double u0=adapt.FirstParameter();
    double u1=0.9*adapt.LastParameter();
    double um=0.5*(u0+u1);
    gp_Pnt p1=adapt.Value(u0), p2=adapt.Value(um), p3=adapt.Value(u1);
    gp_Circ ic = gce_MakeCirc(p1, p2, p3);
    double Lref=std::max( std::max(p1.Distance(p2), p2.Distance(p3)), p1.Distance(p3) );

    // check
    GeomAPI_ExtremaCurveCurve ec(
          curve,
          Handle_Geom_Curve(new Geom_Circle(ic))
          );

    double max_dist=0.0;
    for (int i=1; i<=ec.NbExtrema(); i++)
    {
      max_dist=std::max(ec.Distance(i), max_dist);
    }
    if (max_dist>0.1*Lref)
    {
      throw insight::Exception(boost::str(boost::format
         ("selected edge is a BSplineCurve and possibly not circular! (max. distance=%g)") % max_dist
       ));
    }

    D=ic.Radius()*2.;
  }
  else
    throw insight::Exception("selected edge is not a circle or BSplineCurve! (instead is of type "+boost::lexical_cast<std::string>(adapt.GetType())+")");

  return D;
}




insight::cad::CylinderDiameter::CylinderDiameter(const CylinderDiameter &o, TreeCloneMap &tcm)
    : CL(pfs_)
{}

insight::cad::CylinderDiameter::CylinderDiameter(insight::cad::ConstFeatureSetPtr pfs)
    : pfs_(pfs)
{}



size_t insight::cad::CylinderDiameter::calcHash() const
{
    ParameterListHash h;
    h+=*pfs_;
    return h.getHash();
}

double insight::cad::CylinderDiameter::calcValue() const
{
    if (!(pfs_->size()==1))
        throw insight::Exception("face feature set must not contain more than one face for diameter extraction!");

    FeatureID i=*(pfs_->data().begin());

    TopLoc_Location loc;
    Handle_Geom_Surface surf(BRep_Tool::Surface(pfs_->model()->face(i), loc));
    GeomAdaptor_Surface adapt(surf);

    double D;
    if (adapt.GetType()==GeomAbs_Cylinder)
    {
        auto icyl=adapt.Cylinder();
        D=icyl.Radius()*2.;
    }/*
    else if (adapt.GetType()==GeomAbs_BSplineCurve)
    {
        double u0=adapt.FirstParameter();
        double u1=0.9*adapt.LastParameter();
        double um=0.5*(u0+u1);
        gp_Pnt p1=adapt.Value(u0), p2=adapt.Value(um), p3=adapt.Value(u1);
        gp_Circ ic = gce_MakeCirc(p1, p2, p3);
        double Lref=std::max( std::max(p1.Distance(p2), p2.Distance(p3)), p1.Distance(p3) );

        // check
        GeomAPI_ExtremaCurveCurve ec(
            curve,
            Handle_Geom_Curve(new Geom_Circle(ic))
            );

        double max_dist=0.0;
        for (int i=1; i<=ec.NbExtrema(); i++)
        {
            max_dist=std::max(ec.Distance(i), max_dist);
        }
        if (max_dist>0.1*Lref)
        {
            throw insight::Exception(boost::str(boost::format
                                                ("selected edge is a BSplineCurve and possibly not circular! (max. distance=%g)") % max_dist
                                                ));
        }

        D=ic.Radius()*2.;
    }*/
    else
        throw insight::Exception("selected face is not a cylinder! (instead is of type "+boost::lexical_cast<std::string>(adapt.GetType())+")");

    return D;
}
