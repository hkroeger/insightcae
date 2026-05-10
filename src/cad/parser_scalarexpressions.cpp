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

#include "boost/phoenix/core/reference.hpp"
#include "cadtypes.h"
#ifdef INSIGHT_CAD_DEBUG
#define BOOST_SPIRIT_DEBUG
#endif

#include "cadfeature.h"

#include "datum.h"
#include "featureset.h"
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

#include "parser_tools.h"

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
    r_scalarFunction =
        ( current_pos.current_pos
         >> omit[ scalarFunctionRules [ qi::_a = qi::_1 ] ]
         >> current_pos.current_pos )
            [ phx::at_c<0>(qi::_val) = qi::_1,
              phx::at_c<1>(qi::_val) = qi::_2 ]
         > qi::lazy(*qi::_a)
            [ phx::at_c<2>(qi::_val) = qi::_1 ]
        ;
    r_scalarFunction.name("scalar function");



#define ADD_SCALAR_FUNCTION(NAME, RULE, ACT) \
    scalarFunctionRules.add( \
        NAME, std::make_shared<ScalarFunctionRule>( \
        RULE ACT ))

    ADD_SCALAR_FUNCTION(
        "volume",
        ( '(' > r_solidmodel_expression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<FeatureVolume>(qi::_1)) ]
        );

    ADD_SCALAR_FUNCTION(
        "surfacearea",
        ( '(' > r_solidmodel_expression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<FeatureSurfaceArea>(qi::_1)) ]
        );

    ADD_SCALAR_FUNCTION(
        "cumedgelen",
        ( '(' > r_solidmodel_expression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<CumulativeEdgeLength>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "circdiameter",
        ('(' > r_edgeFeaturesExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<CircleDiameter>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "cyldiameter",
        ('(' > r_faceFeaturesExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<CylinderDiameter>(qi::_1)) ] );


    ADD_SCALAR_FUNCTION(
        "mag",
        ( '(' > r_vectorExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<VectorMag>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "pos",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_pos>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "neg",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_neg>(qi::_1)) ]);

    ADD_SCALAR_FUNCTION(
        "sgn",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sgn>(qi::_1)) ]);

    ADD_SCALAR_FUNCTION(
        "sqrt",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sqrt>(qi::_1)) ]);

    ADD_SCALAR_FUNCTION(
        "sin",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_sin>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "cos",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_cos>(qi::_1)) ]);

    ADD_SCALAR_FUNCTION(
        "tan",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_tan>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "asin",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_asin>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "acos",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_acos>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "ceil",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_ceil>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "floor",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_floor>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "round",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_round>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "pow",
        ( '(' > r_scalarExpression > ',' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_pow>(qi::_1, qi::_2)) ] );

    ADD_SCALAR_FUNCTION(
        "min",
        ( '(' > r_scalarExpression > ',' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_min>(qi::_1, qi::_2)) ] );

    ADD_SCALAR_FUNCTION(
        "max",
        ( '(' > r_scalarExpression > ',' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_max>(qi::_1, qi::_2)) ] );

    ADD_SCALAR_FUNCTION(
        "atan2",
        ( '(' > r_scalarExpression > ',' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_atan2>(qi::_1, qi::_2)) ] );

    ADD_SCALAR_FUNCTION(
        "atan",
        ( '(' > r_scalarExpression > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<Scalar_atan>(qi::_1)) ] );

    ADD_SCALAR_FUNCTION(
        "TableLookup",
        ( '(' > r_identifier > ','
           > r_identifier > ','
           > r_scalarExpression > ','
           > r_identifier
           > ( ( ',' > lit("nearest") > qi::attr(true) ) | qi::attr(false) )
           > ')' ),
        [ _val = phx::construct<ScalarPtr>(phx::new_<LookupTableScalar>(
             qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ] );

#undef ADD_SCALAR_FUNCTION

    r_scalarExpression =
        r_scalar_term [ _val = qi::_1]
        >> *(
              ( '+' > r_scalar_term )
                [ _val = phx::construct<ScalarPtr>(phx::new_<AddedScalar>(qi::_val, qi::_1)) ]
            | ( '-' > r_scalar_term )
                [ _val = phx::construct<ScalarPtr>(phx::new_<SubtractedScalar>(qi::_val, qi::_1)) ]
            )
        ;
    r_scalarExpression.name("scalar expression");

    r_scalar_term =
        (
            r_scalar_primary [ _val = qi::_1 ]
            >> *(
               ( '*' >> r_scalar_primary )
                [ _val = phx::construct<ScalarPtr>(phx::new_<MultipliedScalar>(qi::_val, qi::_1)) ]
             | ( '/' >> r_scalar_primary )
                [ _val = phx::construct<ScalarPtr>(phx::new_<DividedScalar>(qi::_val, qi::_1)) ]
            )
        )
        | ( r_vector_primary >> '&' >> r_vector_primary )
         [ _val = phx::construct<ScalarPtr>(phx::new_<DotMultipliedVector>(qi::_1, qi::_2)) ]
        ;
    r_scalar_term.name("scalar term");


    r_scalar_primary =
        double_
         [ _val = phx::construct<ScalarPtr>(phx::new_<ConstantScalar>(qi::_1)) ]

        | ('(' >> r_scalarExpression >> ')')
         [ _val = qi::_1 ]

        | ('-' >> r_scalar_primary ) // expressions starting with minus
         [ _val = phx::construct<ScalarPtr>(phx::new_<MultipliedScalar>(
            ScalarPtr( new ConstantScalar(-1.0)), qi::_1 )) ]

        | ('+' >> r_scalar_primary ) // expressions starting with plus
         [ _val = qi::_1 ]

        | addAdditionalRule( map_lookup_parser(model_->scalars()) ) //model_->scalarSymbols()
         [ qi::_val = qi::_1 ]

        | r_scalarFunction
         [ _val = phx::at_c<2>(qi::_1) ]

        | ( r_vector_primary >> '.' >> 'x' )
         [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 0)) ]

        | ( r_vector_primary >> '.' >> 'y' )
         [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 1)) ]

        | ( r_vector_primary >> '.' >> 'z' )
         [ _val = phx::construct<ScalarPtr>(phx::new_<VectorComponent>(qi::_1, 2)) ]

        | ( r_solidmodel_expression >> '$' >> r_identifier )
         [ _val = phx::construct<ScalarPtr>(phx::new_<ScalarFeatureProp>(qi::_1, qi::_2)) ]

        ;
    r_scalar_primary.name("scalar primary");

}
    
    
}
}
}
