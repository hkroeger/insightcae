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

void ISCADParser::createSelectionExpressions()
{
    r_vertexFeaturesExpression =
        (
            model_->vertexFeatureSymbols() [ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> '?' ) [qi::_a=qi::_1]
            >> (
               ( (lit("vertices")|lit("vertex"))
                 > (
                  ( lit("at") > r_vectorExpression
                  ) [ _val = phx::bind(&makeVertexFeatureSet,
                        qi::_a, std::string("dist(loc,%m0)<1e-6"),
                        phx::construct<FeatureSetParserArgList>(1, qi::_1) ) ]
                  |
                  ( '('
                   > r_string
                   > *( ',' > (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) )
                   > ')'
                  ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Vertex, qi::_1, qi::_2)) ]
                 )
               )
               |
               ( lit("allvertices")
               ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Vertex)) ]
               |
               ( lit("vid") > '=' > '(' > ( qi::int_ % ',' ) > ')'
               ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Vertex, qi::_1)) ]
            )
        )
        >>
        *(
            '?'
            > (lit("vertices")|lit("vertex"))
            > '('
            > r_string
            > *( ',' > (r_vertexFeaturesExpression|r_vectorExpression|r_scalarExpression) )
            > ')'
        ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
        ;
    r_vertexFeaturesExpression.name("vertex selection expression");

    r_edgeFeaturesExpression =
        (
            model_->edgeFeatureSymbols() [ _val = qi::_1 ]
            |
            (r_solidmodel_expression >> '?') [qi::_a=qi::_1]
            >> (
                ( (lit("edges")|lit("edge"))
                > (
                   ( lit("from") > r_solidmodel_expression
                   ) [ _val = phx::bind(&makeEdgeFeatureSet,
                          qi::_a, std::string("isIdentical(%0)"),
                          phx::construct<FeatureSetParserArgList>(1,
                                phx::bind( &Feature::allEdges, qi::_1) ) ) ]
                   |
                   ( '(' > r_string
                    > *( ',' > ((r_vertexFeaturesExpression|r_edgeFeaturesExpression|r_faceFeaturesExpression|r_solidFeaturesExpression)|r_vectorExpression|r_scalarExpression) )
                    > ')'
                    ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Edge, qi::_1, qi::_2)) ]
                  )
                )
                |
                ( lit("eid") > '=' > '(' > ( qi::int_ % ',' ) > ')'
                ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Edge, qi::_1)) ]
                |
                lit("alledges")
                 [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Edge)) ]
              )
        )
        >>
        *(
            '?'
            > (lit("edges")|lit("edge"))
            > '('
            > r_string
            > *( ',' > ((r_vertexFeaturesExpression|r_edgeFeaturesExpression|r_faceFeaturesExpression|r_solidFeaturesExpression)|r_vectorExpression|r_scalarExpression) )
            > ')'
        )
        [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
        ;
    r_edgeFeaturesExpression.name("edge selection expression");

    r_faceFeaturesExpression =
        (
            model_->faceFeatureSymbols()[ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> '?' ) [qi::_a=qi::_1]
             >> (
                 ( (lit("faces")|lit("face"))
                  > (
                      ( lit("from") > r_solidmodel_expression
                       ) [ _val = phx::bind(&makeFaceFeatureSet,
                            qi::_a, std::string("isIdentical(%0)"),
                            phx::construct<FeatureSetParserArgList>(1,
                                phx::bind( &Feature::allFaces, qi::_1) ) ) ]
                      |
                      (
                        '(' > r_string
                       > *( ',' > (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) )
                       > ')' )
                       [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Face, qi::_1, qi::_2)) ]
                    )
               )
               |
               ( lit("fid") > '=' > '(' > ( qi::int_ % ',' ) > ')'
               ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Face, qi::_1)) ]
               |
               ( lit("allfaces")
               ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Face)) ]
            )
        )
        >>
        *(
            '?'
            > (lit("faces")|lit("face"))
            > '('
            > r_string
            > *( ',' > (r_faceFeaturesExpression|r_vectorExpression|r_scalarExpression) )
            > ')'
        )
        [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
        ;
    r_faceFeaturesExpression.name("face selection expression");

    r_solidFeaturesExpression =
        (
            model_->solidFeatureSymbols()[ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> '?' ) [ _a=qi::_1 ]
             >> (
              ( (lit("solids")|lit("solid"))
               > '('
               > r_string
               > *( ',' > (r_solidFeaturesExpression|r_vectorExpression|r_scalarExpression) )
               > ')'
              ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Solid, qi::_1, qi::_2)) ]
              |
              ( lit("allsolids")
              ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Solid)) ]
              |
              ( lit("sid") > '=' > '(' > (qi::int_ % ',' ) > ')'
              )
              [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Solid, qi::_1)) ]
            )
        )
        >>
        *(
            '?'
            > (lit("solids")|lit("solid"))
            > '('
            > r_string
            > *( ',' > (r_solidFeaturesExpression|r_vectorExpression|r_scalarExpression) )
            > ')'
        )
        [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_val, qi::_1, qi::_2)) ]
        ;
    r_solidFeaturesExpression.name("solid selection expression");
}

}
}
}
