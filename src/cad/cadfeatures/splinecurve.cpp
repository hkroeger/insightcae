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

#include "splinecurve.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

    
    
    
defineType(SplineCurve);
addToFactoryTable(Feature, SplineCurve);




SplineCurve::SplineCurve(): Feature()
{}




SplineCurve::SplineCurve(const std::vector<VectorPtr>& pts)
: pts_(pts)
{}




FeaturePtr SplineCurve::create(const std::vector<VectorPtr>& pts)
{
    return FeaturePtr(new SplineCurve(pts));
}




void SplineCurve::build()
{
    TColgp_Array1OfPnt pts_col ( 1, pts_.size() );
    for ( int j=0; j<pts_.size(); j++ ) {
        pts_col.SetValue ( j+1, to_Pnt ( pts_[j]->value() ) );
    }
    GeomAPI_PointsToBSpline splbuilder ( pts_col );
    Handle_Geom_BSplineCurve crv=splbuilder.Curve();
    setShape ( BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ) );
}




void SplineCurve::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SplineCurve",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression % ',' >> ')' ) 
	[ qi::_val = phx::bind(&SplineCurve::create, qi::_1) ]
      
    ))
  );
}




FeatureCmdInfoList SplineCurve::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "SplineCurve",
            "( <vector:p0>, ..., <vector:pn> )",
            "Creates a spline curve through all given points p0 to pn."
        )
    );
}



}
}
