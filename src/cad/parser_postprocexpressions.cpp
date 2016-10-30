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

void ISCADParser::createPostProcExpressions()
{

    /**
     * \page iscad_postprocessing_commands ISCAD Postprocessing Actions
     * The following actions are available:
     * * \subpage iscad_postprocessing_DXF
     * * \subpage iscad_postprocessing_saveas
     * * \subpage iscad_postprocessing_gmsh
     */


    r_viewDef =
        (r_identifier > '('
         > r_vectorExpression > ','
         > r_vectorExpression
         > ( ( ',' >> lit("up") >> r_vectorExpression ) | attr(VectorPtr()) )
         > ( ( ',' >> lit("section") >> qi::attr(true) ) | attr(false) )
         > ( ( ',' >> lit("poly") >> qi::attr(true) ) | attr(false) )
         > ( ( ',' >> lit("skiphl") >> qi::attr(true) ) | attr(false) )
         > ( ( ',' >> lit("add")
               >> (( 'l' > qi::attr(true) )|qi::attr(false))
               >> (( 'r' > qi::attr(true) )|qi::attr(false))
               >> (( 't' > qi::attr(true) )|qi::attr(false))
               >> (( 'b' > qi::attr(true) )|qi::attr(false))
               >> (( 'k' > qi::attr(true) )|qi::attr(false))
             ) |
             ( qi::attr(false) > qi::attr(false) > qi::attr(false) > qi::attr(false) > qi::attr(false) )
           )
         > ')'
        )
        ;
    r_viewDef.name("view definition");

    r_postproc =

        /** \page iscad_postprocessing_DXF DXF: Save DXF drawing.
        *
        * Syntax:
        *
        * <b>DXF(\ref iscad_filename_expression "<filename>") << \ref iscad_feature_expression "<feature:feature to save>"
        *     \ref iscad_identifier_expression "<identifier:viewname>"
        *       (
        *        \ref iscad_vector_expression "<vector:viewon>",
        *        \ref iscad_vector_expression "<vector:viewnormal>"
        *        [, up \ref iscad_vector_expression "<vector:upward direction>"]
        *        [, section]
        *        [, poly]
        *        [, skiphl]
        *        [, add [l] [r] [t] [b] [k] ]
        *       )
        *     [\ref iscad_identifier_expression "<identifier:viewname>" ...] </b>
        *
        */
        ( lit("DXF") > '(' > r_path > ')' > lit("<<") > ( (r_solidmodel_expression >> *r_viewDef) % ',' ) >> ';' )
        [ phx::bind(&Model::addPostprocActionUnnamed, model_,
                    phx::construct<PostprocActionPtr>(new_<DrawingExport>(qi::_1, qi::_2))) ]
        |

        /** \page iscad_postprocessing_saveas saveAs: Save model geometry to file
        *
        * Syntax:
        *
        * <b>saveAs(\ref iscad_filename_expression "<filename>") << \ref iscad_feature_expression "<feature:feature to save>" </b>
        *
        */
        ( lit("saveAs") > '(' > r_path > ')' > lit("<<")
          > r_solidmodel_expression
          > *( r_identifier > '=' > r_faceFeaturesExpression )
          > ';' )
        [ phx::bind(&Model::addPostprocActionUnnamed, model_,
                    phx::construct<PostprocActionPtr>(new_<Export>(qi::_2, qi::_1, qi::_3))) ]
        |
        ( lit("exportSTL") > '(' > r_path > ',' > r_scalarExpression > ')' > lit("<<") > r_solidmodel_expression > ';' )
        [ phx::bind(&Model::addPostprocActionUnnamed, model_,
                    phx::construct<PostprocActionPtr>(new_<Export>(qi::_3, qi::_1, qi::_2))) ]
        |
        ( lit("exportEMesh") > '(' > r_path > ',' > r_scalarExpression > ',' > r_scalarExpression > ')'
          > lit("<<") > r_edgeFeaturesExpression > ';' )
        [ phx::bind(&Model::addPostprocActionUnnamed, model_,
                    phx::construct<PostprocActionPtr>(new_<Export>(qi::_4, qi::_1, qi::_2, qi::_3))) ]
        |

        /** \page iscad_postprocessing_gmsh gmsh: Create a tringular mesh using gmsh
        *
        * Syntax:
        *
        * <b>gmsh(\ref iscad_filename_expression "<filename>") << \ref iscad_feature_expression "<feature:feature to save>" as \ref iscad_identifier_expression "<identifier:mesh name>"
        *   L = ( \ref iscad_scalar_expression "<scalar:maxL>"  \ref iscad_scalar_expression "<scalar:minL>" )
        *  [linear]
        *  vertexGroups ( \ref iscad_identifier_expression "<identifier:vertex group name>" = \ref iscad_vertexfeat_expression "<vertex features:vertex group>" [ \@ \ref iscad_scalar_expression "<scalar:mesh size>" ] ... )
        *  edgeGroups ( \ref iscad_identifier_expression "<identifier:edge group name>" = \ref iscad_edgefeat_expression "<edge features:edge group>" [ \@ \ref iscad_scalar_expression "<scalar:mesh size>" ] ... )
        *  faceGroups ( \ref iscad_identifier_expression "<identifier:face group name>" = \ref iscad_facefeat_expression "<face features:face group>" [ \@ \ref iscad_scalar_expression "<scalar:mesh size>" ] ... )
        *  vertices ( \ref iscad_identifier_expression "<identifier:vertex name>" = \ref iscad_vector_expression "<vector:vertex location>" ... )
        * </b>
        */
        ( lit("gmsh") >> '(' >> r_path >> ')' >> lit("<<")
          >> r_solidmodel_expression >> lit("as") >> r_identifier
          >> ( lit("L") >> '=' >> '(' >> repeat(2)[r_scalarExpression] ) >> ')'
          >> ( ( lit("linear") >> attr(false) ) | attr(true) )
          >> lit("vertexGroups") >> '(' >> *( ( r_identifier >> '=' >> r_vertexFeaturesExpression >> -( '@' > r_scalarExpression ) ) ) >> ')'
          >> lit("edgeGroups") >> '(' >> *( ( r_identifier >> '=' >> r_edgeFeaturesExpression >> -( '@' > r_scalarExpression ) )  ) >> ')'
          >> lit("faceGroups") >> '(' >> *( ( r_identifier >> '=' >> r_faceFeaturesExpression >> -( '@' > r_scalarExpression ) )  ) >> ')'
          >> ( lit("vertices") >> '(' >> *( r_identifier >> '=' >> r_vectorExpression ) >> ')' | attr(NamedVertices()) )
//         >> ( (lit("keeptmpdir")>attr(true)) | attr(false) )
          >> ';' )
        [ phx::bind(&Model::addPostprocActionUnnamed, model_,
                    phx::construct<PostprocActionPtr>(new_<Mesh>(
                                qi::_1,
                                qi::_2, qi::_3,
                                qi::_4,
                                qi::_5,
                                qi::_6,
                                qi::_7,
                                qi::_8,
                                qi::_9
                            ))) ]
        |
        ( lit("SolidProperties") > '(' > r_identifier > ')' > lit("<<") > r_solidmodel_expression > ';' )
        [ phx::bind(&Model::addPostprocAction, model_, qi::_1,
                    phx::construct<PostprocActionPtr>(new_<SolidProperties>(qi::_2)))
        ]
        |
        ( lit("Hydrostatics") > '('
          > r_identifier > ','
          > r_vectorExpression > ',' > r_vectorExpression > ','
          > r_vectorExpression > ',' > r_vectorExpression
          > ')' > lit("<<") > '(' > r_solidmodel_expression > ',' > r_solidmodel_expression > ')' > ';' ) // (1) hull and (2) ship
        [ phx::bind(&Model::addPostprocAction, model_, qi::_1,
                    phx::construct<PostprocActionPtr>(new_<Hydrostatics>(qi::_6, qi::_7, qi::_2, qi::_3, qi::_4, qi::_5)))
        ]
        ;
    r_postproc.name("postprocessing statement");

}

}
}
}
