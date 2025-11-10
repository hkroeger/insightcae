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

    for (auto& kw: {
            "vertices", "vertex", "allvertices", "vid",
            "edges", "edge", "eid", "alledges",
            "faces", "face", "fid", "allfaces",
            "solids", "solid","allsolids","sid" })
    {
        selectionkeywords.add(kw);
    }

    r_vertexFeaturesExpression =
        (
            model_->vertexFeatureSymbols() [ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> current_pos.current_pos >> '?' ) [qi::_a=qi::_1, qi::_b=qi::_2 ]
            >> (
               ( (lit("vertices")|lit("vertex"))
                 > (
                  ( lit("at") > r_vectorExpression > current_pos.current_pos
                  ) [ _val = phx::bind(
                        &DeferredFeatureSet::create
                            <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>                                                                      ,
                        qi::_a, Vertex, std::string("dist(loc,%m0)<1e-6"),
                        phx::construct<FeatureSetParserArgList>(1, qi::_1) ),

                        phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                             phx::construct<SyntaxElementLocation>(
                                 filenameinfo_,
                                 phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                 ),
                             phx::ref(qi::_val)
                             )
                    ]
                  |
                  ( '('
                   > r_string
                   > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                |r_vectorExpression|r_scalarExpression) )
                   > ')' > current_pos.current_pos
                  ) [ _val = phx::bind(
                        &DeferredFeatureSet::create
                            <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>,
                            qi::_a, insight::cad::Vertex, qi::_1, qi::_2),

                        phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                  phx::construct<SyntaxElementLocation>(
                                      filenameinfo_,
                                      phx::construct<SyntaxElementPos>(qi::_b, qi::_3)
                                      ),
                                  phx::ref(qi::_val)
                                  ) ]
                 |
                 ( r_identifier > current_pos.current_pos)
                      [ _val = phx::bind(
                           &ProvidedFeatureSet::create<ConstFeaturePtr, EntityType, const std::string&>,
                           qi::_a, insight::cad::Vertex, qi::_1 ),

                      phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                phx::construct<SyntaxElementLocation>(
                                    filenameinfo_,
                                    phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                    ),
                                phx::ref(qi::_val)
                                ) ]
                )
               )
               |
               ( lit("allvertices") > current_pos.current_pos
               ) [ _val = phx::bind(
                        &DeferredFeatureSet::create
                            <ConstFeaturePtr,EntityType>,
                            qi::_a, insight::cad::Vertex ),

                     phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                               phx::construct<SyntaxElementLocation>(
                                   filenameinfo_,
                                   phx::construct<SyntaxElementPos>(qi::_b, qi::_1)
                                   ),
                               phx::ref(qi::_val)
                               ) ]
               |
               ( lit("vid") > '=' > '(' > ( qi::int_ % ',' ) > ')' > current_pos.current_pos
               ) [ _val = phx::construct<FeatureSetPtr>(
                        phx::new_<FeatureSet>(
                            qi::_a, insight::cad::Vertex, qi::_1 )),

                  phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                            phx::construct<SyntaxElementLocation>(
                                filenameinfo_,
                                phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                ),
                            phx::ref(qi::_val)
                            ) ]
               //qi::lazy(phx::bind(&Feature::featureSymbols, qi::_a, Vertex)) [ qi::_val = qi::_1 ]
            )
        )
        >>
        *(
            ( current_pos.current_pos >> '?' )
            > (lit("vertices")|lit("vertex"))
            > '('
            > r_string
            > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                                |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                                |r_vectorExpression|r_scalarExpression) )
            > ')' > current_pos.current_pos
        ) [ _val = phx::bind(
                &DeferredFeatureSet::create
                    <ConstFeatureSetPtr,const std::string&,const FeatureSetParserArgList&>,
                    qi::_val, qi::_2, qi::_3),

              phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                        phx::construct<SyntaxElementLocation>(
                            filenameinfo_,
                            phx::construct<SyntaxElementPos>(qi::_1, qi::_4)
                            ),
                        phx::ref(qi::_val)
                        ) ]
        ;
    r_vertexFeaturesExpression.name("vertex selection expression");





    r_edgeFeaturesExpression =
        (
            model_->edgeFeatureSymbols() [ _val = qi::_1 ]
            |
            (r_solidmodel_expression >> current_pos.current_pos >> '?') [ qi::_a=qi::_1, qi::_b=qi::_2 ]
            >> (
                ( (lit("edges")|lit("edge"))
                > (
                   ( lit("from") > r_solidmodel_expression > current_pos.current_pos
                   ) [ _val = phx::bind(
                          &DeferredFeatureSet::create
                            <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>                                                                      ,
                            qi::_a, Edge, std::string("isIdentical(%0)"),
                            phx::construct<FeatureSetParserArgList>(1,
                                phx::bind( &Feature::allEdges, qi::_1)) ),

                            phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                      phx::construct<SyntaxElementLocation>(
                                          filenameinfo_,
                                          phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                          ),
                                      phx::ref(qi::_val)
                                      ) ]
                   |
                   ( '(' > r_string
                    > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                                           |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                                           |r_vectorExpression|r_scalarExpression) )
                    > ')' > current_pos.current_pos
                    ) [ _val = phx::bind(
                            &DeferredFeatureSet::create
                            <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>,
                            qi::_a, insight::cad::Edge, qi::_1, qi::_2),

                          phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                    phx::construct<SyntaxElementLocation>(
                                        filenameinfo_,
                                        phx::construct<SyntaxElementPos>(qi::_b, qi::_3)
                                        ),
                                    phx::ref(qi::_val)
                                    ) ]
                   |
                   ( r_identifier > current_pos.current_pos )
                       [ _val = phx::bind(
                            &ProvidedFeatureSet::create<ConstFeaturePtr, EntityType, const std::string&>,
                                qi::_a, insight::cad::Edge, qi::_1 ),

                           phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                     phx::construct<SyntaxElementLocation>(
                                         filenameinfo_,
                                         phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                         ),
                                     phx::ref(qi::_val)
                                     ) ]
                  )
                )
                |
                ( lit("eid") > '=' > '(' > ( qi::int_ % ',' ) > ')' > current_pos.current_pos
                ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(
                    qi::_a, insight::cad::Edge, qi::_1)),

                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_,
                                  phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                  ),
                              phx::ref(qi::_val)
                              ) ]
              )
            |
            ( lit("alledges") > current_pos.current_pos )
                [ _val = phx::bind(
                     &DeferredFeatureSet::create
                     <ConstFeaturePtr,EntityType>,
                     qi::_a, insight::cad::Edge ),

                 phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                           phx::construct<SyntaxElementLocation>(
                               filenameinfo_,
                               phx::construct<SyntaxElementPos>(qi::_b, qi::_1)
                               ),
                           phx::ref(qi::_val)
                           ) ]
        )
        >>
        *(
            ( current_pos.current_pos >> '?' )
            > (lit("edges")|lit("edge"))
            > '('
            > r_string
            > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                              |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                              |r_vectorExpression|r_scalarExpression) )
            > ')' > current_pos.current_pos
        )
        [ _val = phx::bind(
               &DeferredFeatureSet::create
               <ConstFeatureSetPtr,const std::string&,const FeatureSetParserArgList&>,
               qi::_val, qi::_2, qi::_3),

           phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                     phx::construct<SyntaxElementLocation>(
                         filenameinfo_,
                         phx::construct<SyntaxElementPos>(qi::_1, qi::_4)
                         ),
                     phx::ref(qi::_val)
                     ) ]
        ;
    r_edgeFeaturesExpression.name("edge selection expression");






    r_faceFeaturesExpression =
        (
            ( current_pos.current_pos >> model_->faceFeatureSymbols() >> current_pos.current_pos ) [
                                    qi::_val = qi::_2,

                     phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                               phx::construct<SyntaxElementLocation>(
                                   filenameinfo_,
                                   phx::construct<SyntaxElementPos>(qi::_1, qi::_3)
                                   ),
                               phx::ref(qi::_val)
                               ) ]
            |
            ( r_solidmodel_expression >> current_pos.current_pos >> '?' )
                    [qi::_a=qi::_1, qi::_b=qi::_2]
            >> (
                 ( (lit("faces")|lit("face"))
                  > (
                     ( lit("from") > r_solidmodel_expression > current_pos.current_pos
                       ) [ _val =
                          phx::bind(
                              &DeferredFeatureSet::create
                              <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>                                                                      ,
                              qi::_a, Face, std::string("isIdentical(%0)"),
                              phx::construct<FeatureSetParserArgList>(1,
                                phx::bind( &Feature::allFaces, qi::_1) ) ),

                          phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                    phx::construct<SyntaxElementLocation>(
                                        filenameinfo_,
                                        phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                        ),
                                    phx::ref(qi::_val)
                                    ) ]
                      |
                      (
                        '(' > r_string
                       > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                                           |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                                           |r_vectorExpression|r_scalarExpression) )
                       > ')' > current_pos.current_pos )
                       [ _val = phx::bind(
                               &DeferredFeatureSet::create
                               <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>,
                               qi::_a, insight::cad::Face, qi::_1, qi::_2),

                           phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                     phx::construct<SyntaxElementLocation>(
                                         filenameinfo_,
                                         phx::construct<SyntaxElementPos>(qi::_b, qi::_3)
                                         ),
                                     phx::ref(qi::_val)
                                     )  ]
                      |
                      ( r_identifier > current_pos.current_pos )
                           [ _val = phx::bind(
                                &ProvidedFeatureSet::create<ConstFeaturePtr, EntityType, const std::string&>,
                                    qi::_a, insight::cad::Face, qi::_1 ),

                               phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                         phx::construct<SyntaxElementLocation>(
                                             filenameinfo_,
                                             phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                             ),
                                         phx::ref(qi::_val)
                                         ) ]
                    )
               )
               |
               ( lit("fid") > '=' > '(' > ( qi::int_ % ',' ) > ')' > current_pos.current_pos
               ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Face, qi::_1)),

                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_,
                                  phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                  ),
                              phx::ref(qi::_val)
                              ) ]

                //qi::lazy(phx::bind(&Feature::featureSymbols, qi::_a, Face)) [ qi::_val = qi::_1 ]
              |
              ( lit("allfaces") > current_pos.current_pos
               ) [ _val = phx::bind(
                           &DeferredFeatureSet::create
                           <ConstFeaturePtr,EntityType>,
                           qi::_a, insight::cad::Face ),

                       phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                 phx::construct<SyntaxElementLocation>(
                                     filenameinfo_,
                                     phx::construct<SyntaxElementPos>(qi::_b, qi::_1)
                                     ),
                                 phx::ref(qi::_val)
                                 ) ]
            )
        )
        >>
        *(
            ( current_pos.current_pos >> '?' )
            > (lit("faces")|lit("face"))
            > '('
            > r_string
            > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                              |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                              |r_vectorExpression|r_scalarExpression) )
            > ')' > current_pos.current_pos
        )
        [ _val = phx::bind(
                   &DeferredFeatureSet::create
                   <ConstFeatureSetPtr,const std::string&,const FeatureSetParserArgList&>,
                   qi::_val, qi::_2, qi::_3),

           phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                     phx::construct<SyntaxElementLocation>(
                         filenameinfo_,
                         phx::construct<SyntaxElementPos>(qi::_1, qi::_4)
                         ),
                     phx::ref(qi::_val)
                     ) ]
        ;
    r_faceFeaturesExpression.name("face selection expression");





    r_solidFeaturesExpression =
        (
            model_->solidFeatureSymbols()[ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> current_pos.current_pos >> '?' ) [ _a=qi::_1, _b=qi::_2 ]
            >> ( ( (lit("solids")|lit("solid"))
               > (
                   ('(' > r_string
                    > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                                      |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                                      |r_vectorExpression|r_scalarExpression) )
                    > ')' > current_pos.current_pos )
                   [ _val = phx::bind(
                           &DeferredFeatureSet::create
                           <ConstFeaturePtr,EntityType,const std::string&,const FeatureSetParserArgList&>,
                           qi::_a, insight::cad::Solid, qi::_1, qi::_2),

                         phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                   phx::construct<SyntaxElementLocation>(
                                       filenameinfo_,
                                       phx::construct<SyntaxElementPos>(qi::_b, qi::_3)
                                       ),
                                   phx::ref(qi::_val)
                                   ) ]
                |
                   ( r_identifier > current_pos.current_pos )
                     [ _val = phx::bind(
                          &ProvidedFeatureSet::create<ConstFeaturePtr, EntityType, const std::string&>,
                              qi::_a, insight::cad::Solid, qi::_1 ),

                         phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                   phx::construct<SyntaxElementLocation>(
                                       filenameinfo_,
                                       phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                       ),
                                   phx::ref(qi::_val)
                                   ) ]
                )
             )
             |
             ( lit("sid") > '=' > '(' > (qi::int_ % ',' ) > ')'> current_pos.current_pos
              )
                 [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(qi::_a, insight::cad::Solid, qi::_1)),

                  phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                            phx::construct<SyntaxElementLocation>(
                                filenameinfo_,
                                phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                ),
                            phx::ref(qi::_val)
                            ) ]
             |
             ( lit("allsolids")> current_pos.current_pos
              ) [ _val = phx::bind(
                          &DeferredFeatureSet::create
                          <ConstFeaturePtr,EntityType>,
                          qi::_a, insight::cad::Solid ),

                      phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                                phx::construct<SyntaxElementLocation>(
                                    filenameinfo_,
                                    phx::construct<SyntaxElementPos>(qi::_b, qi::_1)
                                    ),
                                phx::ref(qi::_val)
                                ) ]
             )
        )
        >>
        *(
            ( current_pos.current_pos >> '?' )
            > (lit("solids")|lit("solid"))
            > '('
            > r_string
            > *( ',' > (r_solidFeaturesExpression|r_faceFeaturesExpression
                                               |r_edgeFeaturesExpression|r_vertexFeaturesExpression
                                               |r_vectorExpression|r_scalarExpression) )
            > ')' > current_pos.current_pos
        )
        [ _val = _val = phx::bind(
                &DeferredFeatureSet::create
                <ConstFeatureSetPtr,const std::string&,const FeatureSetParserArgList&>,
                qi::_val, qi::_2, qi::_3),

            phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                      phx::construct<SyntaxElementLocation>(
                          filenameinfo_,
                          phx::construct<SyntaxElementPos>(qi::_1, qi::_4)
                          ),
                      phx::ref(qi::_val)
                      ) ]
        ;
    r_solidFeaturesExpression.name("solid selection expression");


}

}
}
}
