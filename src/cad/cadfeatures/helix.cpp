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

#include "helix.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Helix);
addToFactoryTable(Feature, Helix);

Helix::Helix()
: Feature()
{
}


Helix::Helix(VectorPtr p0, VectorPtr p1, ScalarPtr d, ScalarPtr winds)
: p0_(p0), p1_(p1), d_(d), winds_(winds)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=p0_->value();
  h+=p1_->value();
  h+=d_->value();
  h+=winds_->value();
}

void Helix::build()
{
  double l=to_Pnt(p1_->value()).Distance(to_Pnt(p0_->value()).XYZ());
  Handle_Geom_CylindricalSurface aCylinder
  (
    new Geom_CylindricalSurface(gp_Ax3(to_Pnt(p0_->value()), gp_Dir(to_Pnt(p1_->value()).XYZ()-to_Pnt(p0_->value()).XYZ())), 0.5*d_->value())
  );
  

  gp_Lin2d aLine2d(gp_Pnt2d(0.0, 0.0), gp_Dir2d(2.*M_PI, l/winds_->value()));

  double ll=::sqrt(::pow(l,2)+::pow(2.0*M_PI*winds_->value(),2));
  Handle_Geom2d_TrimmedCurve aSegment = GCE2d_MakeSegment(aLine2d, 0.0, ll);
  TopoDS_Edge ec=BRepBuilderAPI_MakeEdge(aSegment, aCylinder, 0.0, ll).Edge();
    
  double p0, p1;
  BRepLib::BuildCurve3d(ec);
  Handle_Geom_Curve crv = BRep_Tool::Curve(ec, p0, p1);
  gp_Pnt p;
  gp_Vec v;
  crv->D1(crv->FirstParameter(), p, v);
  refpoints_["p0"]=vec3(p);
  refvectors_["et0"]=vec3(v);
  crv->D1(crv->LastParameter(), p, v);
  refpoints_["p1"]=vec3(p);
  refvectors_["et1"]=vec3(v);
  
  setShape(ec);
}

void Helix::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Helix",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' >> ruleset.r_vectorExpression >> ',' 
	    >> ruleset.r_vectorExpression >> ',' 
	    >> ruleset.r_scalarExpression >> ',' 
	    >> ruleset.r_scalarExpression >> ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Helix>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
    ))
  );
}

}
}

