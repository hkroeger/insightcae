
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

#include "curvepattern.h"
#include "transform.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    
    
defineType(CurvePattern);
addToFactoryTable(Feature, CurvePattern);




CurvePattern::CurvePattern(): Compound()
{}


  
  
CurvePattern::CurvePattern(FeaturePtr m1, FeaturePtr curve, ScalarPtr delta, ScalarPtr n)
: m1_(m1), curve_(curve), delta_(delta), n_(n)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*m1_;
  h+=*curve_;
  h+=delta_->value();
  h+=n_->value();
}




FeaturePtr CurvePattern::create ( FeaturePtr m1, FeaturePtr curve, ScalarPtr delta, ScalarPtr n )
{
    return FeaturePtr(new CurvePattern(m1, curve, delta, n));
}




void CurvePattern::build()
{

    double delta=delta_->value();
    int n=ceil ( n_->value() );

    if ( !curve_->isSingleWire() ) 
    {
        throw insight::Exception ( "curve feature does not represent a single wire!" );
    }

    TopoDS_Wire w = curve_->asSingleWire();

    BRepAdaptor_CompCurve crv ( w );
    GCPnts_UniformAbscissa part ( crv, delta );

    if ( part.NbPoints() <n ) 
    {
        throw insight::Exception ( boost::str ( boost::format ( "Could not divide curve into enough segments (request was %d, possible is %d)!" ) % n % part.NbPoints() ) );
    }


    gp_Pnt p0;
    gp_Vec tan0;
    crv.D1 ( crv.FirstParameter(), p0, tan0 );
    gp_Vec side ( ( crv.Value ( part.Parameter ( n ) ).XYZ() - p0.XYZ() ).Crossed ( tan0.XYZ() ) );
    side.Normalize();

    gp_Pnt p1;
    gp_Vec tan1;
    crv.D1 ( part.Parameter ( n ), p1, tan1 );

    int j=0;
    CompoundFeatureMap instances;

    refpoints_["p0"]=vec3 ( p0 );
    refpoints_["et0"]=-vec3 ( tan0 );
    refpoints_["p1"]=vec3 ( p1 );
    refpoints_["et1"]=vec3 ( tan1 );

    for ( int i=0; i<n; i++ ) 
    {
        gp_Pnt p;
        gp_Vec tan;
        crv.D1 ( part.Parameter ( i+1 ), p, tan );

        gp_Trsf tr;
        tr.SetTransformation ( gp_Ax3 ( p, tan, side ) );
        tr.Invert();

        components_[str ( format ( "component%d" ) % ( j+1 ) )] = FeaturePtr ( new Transform ( m1_, tr ) );
        j++;
    }

    m1_->unsetLeaf();
    Compound::build();

//   setShape(result);
}




void CurvePattern::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "CurvePattern",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> 
      ',' >> ruleset.r_solidmodel_expression >> 
      ',' >> ruleset.r_scalarExpression >> 
      ',' >> ruleset.r_scalarExpression >> ')' ) 
      [ qi::_val = phx::bind(&CurvePattern::create, qi::_1, qi::_2, qi::_3, qi::_4) ]
      
    ))
  );
}




FeatureCmdInfoList CurvePattern::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "CurvePattern",
         
            "( <feature:base>, <feature:curve>, <scalar:delta>, <scalar:n> )",
         
            "Copies the bease feature base into a linear pattern along a curve feature (curve)."
            " The distance between subsequent copies is delta and n copies are created."
        )
    );
}



}
}
