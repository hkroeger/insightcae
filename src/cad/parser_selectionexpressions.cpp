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

void ISCADParser::createSelectionExpressions()
{

    // optional arguments of a feature set filter expression: ", arg1, arg2, ..."
    // Each argument is accepted only, if it is followed by "," or ")".
    // Otherwise e.g. the scalar "p.x" would be committed to the vector "p"
    // and the expectation of ")" would throw.
    auto& r_featureSetFilterArgs = addAdditionalRule(
        new qi::rule<std::string::iterator, FeatureSetParserArgList(), skip_grammar>(
        *( ',' >
           (
               ( r_solidFeaturesExpression >> &(lit(',')|')') )
             | ( r_faceFeaturesExpression >> &(lit(',')|')') )
             | ( r_edgeFeaturesExpression >> &(lit(',')|')') )
             | ( r_vertexFeaturesExpression >> &(lit(',')|')') )
             | ( r_vectorExpression >> &(lit(',')|')') )
             | ( r_scalarExpression >> &(lit(',')|')') )
           )
         ) ) );
    r_featureSetFilterArgs.name("feature set filter arguments");

    r_vertexFeaturesExpression =
        (
            addAdditionalRule( map_lookup_parser(model_->vertexFeatures()) )  [ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> current_pos.current_pos >> '?' ) [qi::_a=qi::_1, qi::_b=qi::_2 ]
            >> (
               ( (kw("vertices")|kw("vertex"))
                 > (
                  ( kw("at") > r_vectorExpression > current_pos.current_pos
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
                   > r_featureSetFilterArgs
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
               ( kw("allvertices") > current_pos.current_pos
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
               ( kw("vid") > '=' > '(' > ( qi::int_ % ',' ) > ')' > current_pos.current_pos
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
            > (kw("vertices")|kw("vertex"))
            > '('
            > r_string
            > r_featureSetFilterArgs
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
            addAdditionalRule( map_lookup_parser(model_->edgeFeatures()) ) [ _val = qi::_1 ]
            |
            (r_solidmodel_expression >> current_pos.current_pos >> '?')
               [ qi::_a=qi::_1, qi::_b=qi::_2 ]
            >> (
                ( (kw("edges")|kw("edge"))
                > (
                   ( kw("from") > r_solidmodel_expression > current_pos.current_pos
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
                    > r_featureSetFilterArgs
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
                ( kw("eid") > '=' > '(' > ( qi::int_ % ',' ) > ')' > current_pos.current_pos
                ) [ _val = phx::construct<FeatureSetPtr>(phx::new_<FeatureSet>(
                    qi::_a, insight::cad::Edge, qi::_1)),

                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_,
                                  phx::construct<SyntaxElementPos>(qi::_b, qi::_2)
                                  ),
                              phx::ref(qi::_val)
                              ) ]
                |
                ( kw("alledges") > current_pos.current_pos )
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
        )
        >>
        *(
            ( current_pos.current_pos >> '?' )
            > (kw("edges")|kw("edge"))
            > '('
            > r_string
            > r_featureSetFilterArgs
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
            ( current_pos.current_pos >> addAdditionalRule( map_lookup_parser(model_->faceFeatures()) ) >> current_pos.current_pos ) [
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
                 ( (kw("faces")|kw("face"))
                  > (
                     ( kw("from") > r_solidmodel_expression > current_pos.current_pos
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
                       > r_featureSetFilterArgs
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
               ( kw("fid") > '=' > '(' > ( qi::int_ % ',' ) > ')' > current_pos.current_pos
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
              ( kw("allfaces") > current_pos.current_pos
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
            > (kw("faces")|kw("face"))
            > '('
            > r_string
            > r_featureSetFilterArgs
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
            addAdditionalRule( map_lookup_parser(model_->solidFeatures()) ) [ qi::_val = qi::_1 ]
            |
            ( r_solidmodel_expression >> current_pos.current_pos >> '?' ) [ _a=qi::_1, _b=qi::_2 ]
            >> ( ( (kw("solids")|kw("solid"))
               > (
                   ('(' > r_string
                    > r_featureSetFilterArgs
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
             ( kw("sid") > '=' > '(' > (qi::int_ % ',' ) > ')'> current_pos.current_pos
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
             ( kw("allsolids")> current_pos.current_pos
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
            > (kw("solids")|kw("solid"))
            > '('
            > r_string
            > r_featureSetFilterArgs
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
