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

#include "torus.h"
#include "base/boost_include.h"

#include "BRepPrimAPI_MakeTorus.hxx"

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    
    
defineType(Torus);
addToFactoryTable(Feature, Torus);




Torus::Torus()
{}




Torus::Torus(VectorPtr p0, VectorPtr axisTimesD, ScalarPtr d)
: p0_(p0), axisTimesD_(axisTimesD), d_(d)
{
    ParameterListHash h(this);
    h+=this->type();
    h+=p0_->value();
    h+=axisTimesD_->value();
    h+=d_->value();
}




FeaturePtr Torus::create ( VectorPtr p0, VectorPtr axisTimesD, ScalarPtr d )
{
    return FeaturePtr(new Torus(p0, axisTimesD, d));
}




void Torus::build()
{
    double D=arma::norm ( axisTimesD_->value(), 2 );
    arma::mat axis=axisTimesD_->value() /D;

    refpoints_["p0"]=p0_->value();
    refvectors_["axis"]=axis;
    refvalues_["D"]=D;
    refvalues_["d"]=d_->value();

    TopoDS_Shape tor=
        BRepPrimAPI_MakeTorus
        (
            gp_Ax2
            (
                to_Pnt ( p0_->value() ),
                gp_Dir ( to_Vec ( axis ) )
            ),
            0.5*D,
            0.5*d_->value()
        ).Shape();

    setShape ( tor );
}




void Torus::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Torus",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_scalarExpression 
      >> ')' ) 
     [ qi::_val = phx::bind(&Torus::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Torus::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Torus",
            "( <vector:p0>, <vector:axisTimesD>, <scala:d> )",
            "Creates a torus around center point p0. The axis is specified by the direction of axisTimesD and the torus diameter by its magnitude. The tube diameter is d."
        )
    );
}




}
}
