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

#include "coincidentprojectededge.h"
#include "cadfeatures.h"
#include "datum.h"


using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{

coincidentProjectedEdge::coincidentProjectedEdge
(
  ConstFeaturePtr m, 
  const matQuantityComputerPtr& p0, 
  const matQuantityComputerPtr& n, 
  const matQuantityComputerPtr& up,
  const scalarQuantityComputerPtr& tol
)
: f_(m, Edge),
  p0_(p0), n_(n), up_(up), tol_(tol)
{
}

coincidentProjectedEdge::coincidentProjectedEdge
(
  FeatureSet f, 
  const matQuantityComputerPtr& p0, 
  const matQuantityComputerPtr& n, 
  const matQuantityComputerPtr& up,
  const scalarQuantityComputerPtr& tol
)
: f_(f),
  p0_(p0), n_(n), up_(up), tol_(tol)
{
}

void coincidentProjectedEdge::firstPass(FeatureID feature)
{
  if (samplePts_.n_cols==0)
  {
    double tol=tol_->evaluate(feature);
    
    BOOST_FOREACH(FeatureID f, f_.data())
    {    
      TopoDS_Edge e=TopoDS::Edge(f_.model()->edge(f));
      BRepAdaptor_Curve ac(e);
      GCPnts_QuasiUniformDeflection qud(ac, 0.1*tol);

      arma::mat mypts=arma::zeros<arma::mat>(3,qud.NbPoints());
      for (int j=1; j<=qud.NbPoints(); j++)
      {
	arma::mat p=insight::Vector(qud.Value(j)).t();
	mypts.col(j-1)=p;
      }
      
      if (samplePts_.n_cols==0)
	samplePts_=mypts;
      else
	samplePts_=join_rows(samplePts_, mypts);
      
//       cout <<"f="<<f<<", #cols="<<samplePts_.n_cols<<endl;
    }
    
//     arma::mat spt=samplePts_.t();
//     spt.save("samplePts.csv", arma::raw_ascii);
  }
}


bool coincidentProjectedEdge::checkMatch(FeatureID feature) const
{
  DatumPtr pln(new DatumPlane
  (
    VectorPtr(new ConstantVector(p0_->evaluate(feature))), 
    VectorPtr(new ConstantVector(n_->evaluate(feature))), 
    VectorPtr(new ConstantVector(up_->evaluate(feature)))
  ));
  double tol=tol_->evaluate(feature);
  
  TopoDS_Edge e1=model_->edge(feature);
  FeaturePtr se1(new Feature(e1));
  FeaturePtr po(new ProjectedOutline(se1, pln));
  TopoDS_Shape pe=po->shape();
  
  bool match=true;
  for (TopExp_Explorer ex(pe, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge e=TopoDS::Edge(ex.Current());
    BRepAdaptor_Curve ac(e);
    GCPnts_QuasiUniformDeflection qud(ac, tol);

    for (int j=1; j<=qud.NbPoints(); j++)
    {
      arma::mat p=insight::Vector(qud.Value(j)).t();

      arma::mat d=samplePts_ - p*arma::ones<arma::mat>(1,samplePts_.n_cols);
      arma::mat ds=arma::sqrt( d.row(0)%d.row(0) + d.row(1)%d.row(1) + d.row(2)%d.row(2) );

      double md=arma::as_scalar(arma::min(ds, 1));
      
      match=match & (md<tol);
    }
  }
  
  return match;
}

FilterPtr coincidentProjectedEdge::clone() const
{
  return FilterPtr(new coincidentProjectedEdge(f_, p0_, n_, up_, tol_));
}

}
}