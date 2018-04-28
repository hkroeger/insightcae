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

#include "sinewave.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/fusion.hpp>

#include "TColgp_HArray1OfPnt.hxx"
#include "GeomAPI_Interpolate.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {




defineType(SineWave);
addToFactoryTable(Feature, SineWave);



size_t SineWave::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=l_->value();
  h+=A_->value();
  return h.getHash();
}



SineWave::SineWave(): Feature()
{}




SineWave::SineWave(ScalarPtr l, ScalarPtr A )
: l_(l), A_(A)
{}




FeaturePtr SineWave::create(ScalarPtr l, ScalarPtr A )
{
    return FeaturePtr(new SineWave(l, A));
}




void SineWave::build()
{
    int np=20;
    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, np );
    for ( int j=0; j<np; j++ ) {
        double x=double(j)/double(np-1);
        arma::mat pi=vec3(l_->value()*x, A_->value()*::sin(2.*M_PI*x), 0. );
        pts_col->SetValue ( j+1, to_Pnt ( pi ) );
        refpoints_[str(format("p%d")%j)]=pi;
    }
//     GeomAPI_PointsToBSpline splbuilder ( pts_col );
    GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
//    if (tan0_ && tan1_)
//    {
//        splbuilder.Load(to_Vec(tan0_->value()), to_Vec(tan1_->value()));
//    }
    splbuilder.Perform();
    Handle_Geom_BSplineCurve crv=splbuilder.Curve();
    setShape ( BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ) );
}




void SineWave::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "SineWave",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '(' > ruleset.r_scalarExpression >> ',' >> ruleset.r_scalarExpression >> ')' )
        [ qi::_val = phx::bind(&SineWave::create, qi::_1, qi::_2 ) ]

    ))
  );
}




FeatureCmdInfoList SineWave::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "SineWave",
            "( <scalar:l>, <scalar:A> )",
            "Creates a sine curve (single period) with wave length l and amplitude A."
        )
    );
}



}
}
