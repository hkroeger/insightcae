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

#include "coil.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Coil);
addToFactoryTable(Feature, Coil, NoParameters);

Coil::Coil(const NoParameters& nop)
: Feature(nop)
{
}


Coil::Coil
(
  VectorPtr p0,
  VectorPtr b,
  VectorPtr l,
  ScalarPtr r,
  ScalarPtr d,
  ScalarPtr nv, 
  ScalarPtr nr
)
: p0_(p0), b_(b), l_(l), r_(r), d_(d), nv_(nv), nr_(nr)
{}

void Coil::build()
{
  
  arma::mat p0=p0_->value();
  arma::mat b=b_->value();
  arma::mat l=l_->value();
  arma::mat eb=b/arma::norm(b,2);
  arma::mat el=l/arma::norm(l,2);
  arma::mat ez=arma::cross(el, eb);
  
  double r=r_->value();
  double d=d_->value();
  
  if (r>norm(b,2)/2)
    throw insight::Exception("Invalid definition: coil bending radius r>width/2!");
  
  double cb=norm(b,2)+d;
  arma::mat lp0=p0+0.5*l+0.5*cb*eb;
  
  BRepBuilderAPI_MakeWire wb;

  int vdir=-1;
  int nr=int(nr_->value());
  int nv=int(nv_->value());
  double cl=norm(l,2);
  
  for (int j=0; j<nr; j++)
  {
    vdir*=-1;
    for (int i=0; i<nv; i++)
    {
      
      arma::mat p[9], m[4];
      p[0]=lp0;
      
      p[1]=p[0]-cl*el;
      m[1]=p[1]-r*eb;
      p[2]=m[1]-r*el;
      
      p[4]=p[1]-cb*eb;
      m[2]=p[4]+r*eb;
      p[3]=m[2]-r*el;
      
      arma::mat ebu=eb;
      double fac=1.;
      if (i<(nv-1)) 
      {
	ebu=cb*eb+vdir*d*ez;
	fac=1./::cos(atan(d/cb));
      }
      else
      {
	cb+=0.5*d;
	cl+=0.5*d;
      }
      ebu/=norm(ebu,2);
      p[5]=p[4]+cl*el;
      m[3]=p[5]+(r*fac)*ebu;
      p[6]=m[3]+r*el;
      p[7]=p[6]+((cb-2*r)*fac)*ebu;
      m[0]=p[7]-r*el;
      if (i==(nv-1)) 
      {
	p[7]+=d*ebu;
	cb+=0.5*d;
	cl+=0.5*d;
      }
      
      
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[0]), to_Pnt(p[1])).Value()).Edge());
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[1]), to_Vec(-el), to_Pnt(p[2])).Value()));
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[2]), to_Pnt(p[3])).Value()).Edge());
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[4]), to_Vec(-el), to_Pnt(p[3])).Value()));
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[4]), to_Pnt(p[5])).Value()).Edge());
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[5]), to_Vec(el), to_Pnt(p[6])).Value()));
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p[6]), to_Pnt(p[7])).Value()).Edge());
      
      arma::mat np0=m[0]+r*ebu;
      wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(to_Pnt(p[7]), to_Vec(ebu), to_Pnt(np0)).Value()));
      lp0=np0;
  //     cb+=d;
    }
  }
  
  setShape(wb.Wire());
}

void Coil::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Coil",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Coil>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7)) ]
    ))
  );
}

bool Coil::isSingleCloseWire() const
{
  return false;
}

bool Coil::isSingleOpenWire() const
{
  return true;
}

}
}