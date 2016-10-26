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

#include "closedpolyline.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    
    
defineType(ClosedPolyline);
addToFactoryTable(Feature, ClosedPolyline, NoParameters);




ClosedPolyline::ClosedPolyline(const NoParameters&)
{}




ClosedPolyline::ClosedPolyline(std::vector<VectorPtr> pts)
: pts_(pts)
{}




FeaturePtr ClosedPolyline::create(std::vector<VectorPtr> pts)
{
    return FeaturePtr(new ClosedPolyline(pts));
}




void ClosedPolyline::build()
{
    BRepBuilderAPI_MakeWire w;
    for (int i=1; i<pts_.size(); i++)
    {
        gp_Pnt p0(to_Pnt(pts_[i-1]->value()));
        gp_Pnt p1(to_Pnt(pts_[i]->value()));
        w.Add(BRepBuilderAPI_MakeEdge(p0, p1));
    }
    {
        gp_Pnt p0(to_Pnt(pts_[pts_.size()-1]->value()));
        gp_Pnt p1(to_Pnt(pts_[0]->value()));
        w.Add(BRepBuilderAPI_MakeEdge(p0, p1));
    }

    providedSubshapes_["OuterWire"]=FeaturePtr(new Feature(w.Wire()));

    setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}




void ClosedPolyline::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "ClosedPolyline",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression%',' > ')' ) 
	[ qi::_val = phx::bind(&ClosedPolyline::create, qi::_1) ]
      
    ))
  );
}




FeatureCmdInfoList ClosedPolyline::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "ClosedPolyline",
         
            "( <vector:p0>, <vector:p1>, ..., <vector:pn> )",
         
            "Creates a closed polyline from the given list of points p0 to pn."
        )
    );
}



}
}
