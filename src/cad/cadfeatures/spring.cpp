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

#include "spring.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Spring);
addToFactoryTable(Feature, Spring);

Spring::Spring()
: Feature()
{
}


Spring::Spring(VectorPtr p0, VectorPtr p1, ScalarPtr d, ScalarPtr winds)
: p0_(p0), p1_(p1), d_(d), winds_(winds)
{}

void Spring::build()
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
    
  BRepBuilderAPI_MakeWire wb;
  wb.Add(ec);
  wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p0_->value()), BRep_Tool::Pnt(TopExp::FirstVertex(ec)))));
  wb.Add(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p1_->value()), BRep_Tool::Pnt(TopExp::LastVertex(ec)))));
//   BOOST_FOREACH(const FeatureID& fi, edges)
//   {
//     wb.Add(edges.model().edge(fi));
//   }
  setShape(wb.Wire());
}

void Spring::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Spring",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
      ( '(' > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_scalarExpression > ',' 
	    > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Spring>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
    ))
  );
}

}
}
