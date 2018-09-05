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

#include "spiral.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "TColgp_HArray1OfPnt.hxx"
#include "GeomAPI_Interpolate.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Spiral);
addToFactoryTable(Feature, Spiral);

size_t Spiral::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p0_->value();
  h+=axis_->value();
  h+=n_->value();
  h+=a_->value();
  h+=P_->value();
  return h.getHash();
}



Spiral::Spiral()
: Feature()
{
}


Spiral::Spiral(VectorPtr p0, VectorPtr axis, ScalarPtr n, ScalarPtr a, ScalarPtr P)
: p0_(p0), axis_(axis), n_(n), a_(a), P_(P)
{
}

void Spiral::build()
{
    arma::mat ax=axis_->value();
    arma::mat er=arma::cross(ax, vec3(1,0,0));
    if (arma::norm(er,2)<1e-3) er=arma::cross(ax, vec3(0,1,0));
    arma::mat et=arma::cross(er, ax);

    double a=a_->value();

    int np=50;
    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, np );
    double phimax = 2.*M_PI*n_->value();
    for (int i=np-1; i>=0; i--)
    {
        double phi = 2.*M_PI*n_->value()*double(i)/double(np-1);
        arma::mat p = p0_->value() + a*phi*( ::cos(phi)*er + ::sin(phi)*et ) +ax*(phimax-phi)*P_->value()/2./M_PI;

        pts_col->SetValue ( i+1, to_Pnt ( p ) );
        refpoints_[str(format("p%d")%i)]=p;
    }

    GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
    splbuilder.Perform();
    Handle_Geom_BSplineCurve crv=splbuilder.Curve();
    setShape ( BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ) );
}

void Spiral::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Spiral",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(
      ( '(' >> ruleset.r_vectorExpression >> ','
            >> ruleset.r_vectorExpression >> ','
            >> ruleset.r_scalarExpression >> ','
            >> ruleset.r_scalarExpression
            >> (( ',' >> ruleset.r_scalarExpression)|qi::attr(scalarconst(0.0))) >> ')' )
        [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Spiral>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
    ))
  );
}

}
}
