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

#include "splinesurface.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(SplineSurface);
addToFactoryTable(Feature, SplineSurface, NoParameters);




SplineSurface::SplineSurface(const NoParameters&)
{}




SplineSurface::SplineSurface(const std::vector< std::vector<VectorPtr> >& pts)
: pts_(pts)
{}



FeaturePtr SplineSurface::create ( const std::vector<std::vector<VectorPtr> >& pts )
{
    return FeaturePtr(new SplineSurface(pts));
}




void SplineSurface::build()
{
    int nx=pts_.size();
    if ( nx<2 ) {
        throw insight::Exception ( "SplineSurface: not enough rows of point specified!" );
    }
    int ny=pts_[0].size();
    if ( ny<2 ) {
        throw insight::Exception ( "SplineSurface: not enough cols of point specified!" );
    }

    TColgp_Array2OfPnt pts_col ( 1, nx, 1, ny );
    for ( int j=0; j<nx; j++ ) {
        if ( pts_[j].size() !=ny ) {
            throw insight::Exception ( "SplineSurface: all rows need to have an equal number of points!" );
        }
        for ( int k=0; k<ny; k++ ) {
            pts_col.SetValue ( j+1, k+1, to_Pnt ( pts_[j][k]->value() ) );
        }
    }
    GeomAPI_PointsToBSplineSurface spfbuilder ( pts_col );
    Handle_Geom_BSplineSurface srf=spfbuilder.Surface();
    setShape ( BRepBuilderAPI_MakeFace ( srf, true ) );
}




SplineSurface::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




void SplineSurface::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SplineSurface",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> 
	  ( ( '(' >> ( ruleset.r_vectorExpression % ',' ) >> ')' ) % ',' )
	  >> ')' ) 
	[ qi::_val = phx::bind(&SplineSurface::create, qi::_1) ]
      
    ))
  );
}




FeatureCmdInfoList SplineSurface::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "SplineSurface",
            "( (<vector:p11>, ..., <vector:p1n>), ...., (<vector:pm1>, ..., <vector:pmn>) )",
            "Creates an spline surface through all the given points. Note that all rows need to have the same number of columns."
        )
    );
}



}
}
