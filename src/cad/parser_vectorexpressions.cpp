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

void ISCADParser::createVectorExpressions()
{

    r_vectorExpression =
        r_vector_term [ _val = qi::_1 ]  >>
        *(
            ( '+' >> r_vector_term [_val=phx::construct<VectorPtr>(phx::new_<AddedVector>(qi::_val, qi::_1)) ] )
            |
            ( '-' >> r_vector_term [_val=phx::construct<VectorPtr>(phx::new_<SubtractedVector>(qi::_val, qi::_1)) ] )
        )
        ;
    r_vectorExpression.name("vector expression");

    r_vector_term =
        (
            r_vector_primary [_val=qi::_1] >> *(
                ( '*' >> r_scalar_term
                  [ _val=phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>(qi::_1, qi::_val)) ] )
                | ( '/' >> r_scalar_term
                    [ _val=phx::construct<VectorPtr>(phx::new_<ScalarDividedVector>(qi::_val, qi::_1)) ] )
                | ( '^' >> r_vector_primary
                    [ _val=phx::construct<VectorPtr>(phx::new_<CrossMultipliedVector>(qi::_val, qi::_1)) ] )
                | lit(">>") >>
                (
                    ( '(' >> r_datumExpression >> ',' >> r_vectorExpression >> ')' )
                    [ _val = phx::construct<VectorPtr>(phx::new_<ProjectedPoint>(qi::_val, qi::_1, qi::_2)) ]
                    |
                    ( '(' >> r_solidmodel_expression >> ',' >> r_vectorExpression >> ')' )
                    [ _val = phx::construct<VectorPtr>(phx::new_<ProjectedPointOnFeature>(qi::_val, qi::_2, qi::_1)) ]
                    |
                    r_datumExpression
                    [ _val = phx::construct<VectorPtr>(phx::new_<ProjectedPoint>(qi::_val, qi::_1)) ]
                )
            )
        ) | (
            r_scalar_primary >> '*' >> r_vector_term
        )  [ _val = phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>(qi::_1, qi::_2)) ]
        ;
    r_vector_term.name("vector term");

    r_vector_primary =
//        ( lit("modelCoG") )
//         [ _val = phx::bind(&Model::modelCoG, model_) ]
//       |
        ( lit("rot") >> '('
          >> r_vectorExpression
          >> lit("by") >> r_scalarExpression
          >> ( (lit("around") >> r_vectorExpression) | attr(VectorPtr( new ConstantVector(vec3(0,0,1)))) ) >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<RotatedVector>(qi::_1, qi::_2, qi::_3)) ]
        |
//        qi::lexeme[model_->vectorSymbols()]
//         [ _val =  phx::bind(&Model::lookupVector, model_, qi::_1) ]
        model_->vectorSymbols()[ qi::_val = qi::_1 ]
//       qi::lexeme[ model_->modelsteps() ] [ _a =  phx::bind(&Model::lookupModelstep, model_, qi::_1) ]
// 	  >> lit("->") >
// 	   (
// 	     lit("CoG") [ lazy( _val = phx::bind(&getModelCoG, *_a)) ]
// 	   )
        |
        ( lit("Mechanism_CrankDrive") >> '('
          >> r_scalarExpression >> ','
          >> r_vectorExpression >> ','
          >> r_scalarExpression >> ','
          >> r_vectorExpression >> ','
          >> r_vectorExpression
          >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<Mechanism_CrankDrive>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
        |
        ( lit("Mechanism_Slider") >> '('
          >> r_scalarExpression >> ','
          >> r_vectorExpression >> ','
          >> r_vectorExpression >> ','
          >> r_vectorExpression
          >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<Mechanism_Slider>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
        |
        ( lit("coord") >> '(' >>
          r_vertexFeaturesExpression [ _val = phx::construct<VectorPtr>(phx::new_<SinglePointCoords>(qi::_1)) ]
          >> ')' )
        |
        ( lit("bbmin") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<BBMin>(qi::_1)) ]
        |
        ( lit("bbmax") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<BBMax>(qi::_1)) ]
        |
        ( lit("cog") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<COG>(qi::_1)) ]
        |
        ( lit("surfcog") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<SurfaceCOG>(qi::_1)) ]
        |
        ( lit("surfinert1") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<SurfaceInertiaAxis>(qi::_1, 0)) ]
        |
        ( lit("surfinert2") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<SurfaceInertiaAxis>(qi::_1, 1)) ]
        |
        ( lit("surfinert3") >> '(' >> r_solidmodel_expression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<SurfaceInertiaAxis>(qi::_1, 2)) ]
        |
        ( lit("scoord") >> '(' >>
          r_solidmodel_expression [ _val = phx::construct<VectorPtr>(phx::new_<SinglePointCoords>(
                  phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_1, insight::cad::Vertex))
                                           )) ]
          >> ')' )

        |
        ( /*lit("refpt") >> '(' >>*/ r_datumExpression /*>> ')'*/ )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumPointCoord>(qi::_1)) ]
        |
        ( lit("refpt") >> '(' >> r_datumExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumPointCoord>(qi::_1)) ]
        |
        ( lit("refdir") >> '(' >> r_datumExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumDir>(qi::_1)) ]
        |
        ( lit("plnorm") >> '(' >> r_datumExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<DatumPlaneNormal>(qi::_1)) ]
        |
        ( lit("circcenter") >> '(' >> r_edgeFeaturesExpression >> ')' )
        [ _val = phx::construct<VectorPtr>(phx::new_<CircleEdgeCenterCoords>(qi::_1)) ]
        |
        ( "[" >> r_scalarExpression >> "," >> r_scalarExpression >> "," >> r_scalarExpression >> "]" )
        [ _val = phx::construct<VectorPtr>(phx::new_<VectorFromComponents>(qi::_1, qi::_2, qi::_3)) ]
        //| ( r_vectorExpression >> '\'') [ _val = trans_(qi::_1) ]
        |
        ( '(' >> r_vectorExpression >> ')' )
        [_val = qi::_1]
        |
        ( '-' >> r_vector_primary )
        [ _val = phx::construct<VectorPtr>(phx::new_<ScalarMultipliedVector>( ScalarPtr( new ConstantScalar(-1.0)), qi::_1 )) ]
        |
        ( r_solidmodel_expression >> '@' >> /*(*/ r_identifier /*| qi::attr(std::string()) )*/ )
        [ _val = phx::construct<VectorPtr>(phx::new_<PointFeatureProp>(qi::_1, qi::_2)) ]
        |
        ( r_solidmodel_expression >> '!' >> /*(*/ r_identifier /*| qi::attr(std::string()) )*/ )
        [ _val = phx::construct<VectorPtr>(phx::new_<VectorFeatureProp>(qi::_1, qi::_2)) ]
        ;
    r_vector_primary.name("vector primary");
    
}

}
}
}
