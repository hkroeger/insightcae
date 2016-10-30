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
 *
 */

#include "cadtypes.h"
#ifdef INSIGHT_CAD_DEBUG
#define BOOST_SPIRIT_DEBUG
#endif

#include "cadfeature.h"

#include "datum.h"
#include "sketch.h"
#include "cadpostprocactions.h"

#include "base/analysis.h"
#include "parser.h"
#include "boost/locale.hpp"
#include "base/boost_include.h"
#include "boost/make_shared.hpp"
#include <boost/fusion/adapted.hpp>
#include <boost/phoenix/fusion.hpp>

#include "cadfeatures.h"
#include "meshing.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


namespace insight {
namespace cad {
namespace parser {
    
using namespace qi;
using namespace phx;
using namespace insight::cad;

void ISCADParser::createScalarExpressions()
{

    r_scalarExpression =
        r_scalar_term [ _val = qi::_1]  >> *(
            ( '+' >> r_scalar_term [ _val = phx::construct<ScalarPtr>(phx::new_<AddedScalar>(qi::_val, qi::_1)) ] )
            | ( '-' >> r_scalar_term [ _val = phx::construct<ScalarPtr>(phx::new_<SubtractedScalar>(qi::_val, qi::_1)) ] )
        )
        ;
    r_scalarExpression.name("scalar expression");

    r_scalar_term =
        (
            r_scalar_primary [ _val = qi::_1 ] >>
            *(
                ( '*' >> r_scalar_primary [ _val = phx::construct<ScalarPtr>(phx::new_<MultipliedScalar>(qi::_val, qi::_1)) ] )
                |
                ( '/' >> r_scalar_primary [ _val = phx::construct<ScalarPtr>(phx::new_<DividedScalar>(qi::_val, qi::_1)) ] )
            )
        )
        |
        (
            r_vector_primary >> '&' >> r_vector_primary
        ) [ _val = phx::construct<ScalarPtr>(phx::new_<DotMultipliedVector>(qi::_1, qi::_2)) ]
        ;
    r_scalar_term.name("scalar term");

    r_scalar_primary =
        double_
        [ _val = phx::construct<ScalarPtr>(phx::new_<ConstantScalar>(qi::_1)) ]
        |
//        qi::lexeme[model_->scalarSymbols()]
// 	[ _val = phx::bind(&Model::lookupScalar, model_, qi::_1) ]
        model_->scalarSymbols()[ qi::_val = qi::_1 ]
        | ( lit("volume") > '(' > r_solidmodel_expression > ')' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<FeatureVolume>(qi::_1)) ]
        | ( lit("cumedgelen") > '(' > r_solidmodel_expression > ')' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<CumulativeEdgeLength>(qi::_1)) ]
        | ( lit("mag") > '(' > r_vectorExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<VectorMag>(qi::_1)) ]
        | ( lit("sqrt") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sqrt>(qi::_1)) ]
        | ( lit("sin") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sin>(qi::_1)) ]
        | ( lit("cos") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_cos>(qi::_1)) ]
        | ( lit("tan") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_tan>(qi::_1)) ]
        | ( lit("asin") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_asin>(qi::_1)) ]
        | ( lit("acos") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_acos>(qi::_1)) ]
        | ( lit("ceil") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_ceil>(qi::_1)) ]
        | ( lit("floor") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_floor>(qi::_1)) ]
        | ( lit("round") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_round>(qi::_1)) ]
        | ( lit("pow") > '(' > r_scalarExpression > ',' > r_scalarExpression > ')' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_pow>(qi::_1, qi::_2)) ]
        | ( lit("atan2") > '(' > r_scalarExpression > ',' > r_scalarExpression > ')' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_atan2>(qi::_1, qi::_2)) ]
        | ( lit("atan") > '(' > r_scalarExpression > ')' ) [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_atan>(qi::_1)) ]
        | ('(' >> r_scalarExpression >> ')') [ _val = qi::_1 ]

        | ( lit("TableLookup") > '(' > r_identifier > ','
            > r_identifier > ',' > r_scalarExpression > ',' > r_identifier
            > ( ( ',' >> lit("nearest") >> qi::attr(true) ) | qi::attr(false) )
            > ')' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<LookupTableScalar>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]

        | ( r_vector_primary >> '.' >> 'x' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 0)) ]
        | ( r_vector_primary >> '.' >> 'y' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 1)) ]
        | ( r_vector_primary >> '.' >> 'z' )
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 2)) ]
        | ('-' >> r_scalar_primary)
        [ _val =  phx::construct<ScalarPtr>(phx::new_<MultipliedScalar>( ScalarPtr( new ConstantScalar(-1.0)), qi::_1 )) ]
        | ( r_solidmodel_expression >> '$' >> /*(*/ r_identifier /*| qi::attr(std::string()) )*/ )
        [ _val = phx::construct<ScalarPtr>(phx::new_<ScalarFeatureProp>(qi::_1, qi::_2)) ]
        ;
    r_scalar_primary.name("scalar primary");

}
    
    
}
}
}
