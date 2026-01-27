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
#include "cadpostprocactions/angle.h"
#include "cadpostprocactions/bom.h"

#include "base/analysis.h"
#include "parser.h"
#include "boost/locale.hpp"
#include "base/boost_include.h"
#include "boost/make_shared.hpp"
#include <boost/fusion/adapted.hpp>
#include <boost/phoenix/fusion.hpp>

#include "cadfeatures.h"
#include "meshing.h"
#include "cadpostprocactions/framemesh.h"


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

    for (const auto& apr : PostprocAction::insertrule())
    {
        apr.second(*this);
    }

    /**
     * \page iscad_postprocessing_commands ISCAD Postprocessing Actions
     * The following actions are available:
     * * \subpage iscad_postprocessing_DXF
     * * \subpage iscad_postprocessing_saveas
     * * \subpage iscad_postprocessing_gmsh
     */


    r_viewDef =
        (r_identifier >> '('
         >> r_vectorExpression >> ','
         >> r_vectorExpression
         >> ( ( ',' >> lit("up") >> r_vectorExpression ) | attr(VectorPtr()) )
         >> ( ( ',' >> lit("section") >> qi::attr(true) ) | attr(false) )
         >> ( ( ',' >> lit("poly") >> qi::attr(true) ) | attr(false) )
         >> ( ( ',' >> lit("skiphl") >> qi::attr(true) ) | attr(false) )
         >> ( ( ',' >> lit("add")
               >> (( 'l' >> qi::attr(true) )|qi::attr(false))
               >> (( 'r' >> qi::attr(true) )|qi::attr(false))
               >> (( 't' >> qi::attr(true) )|qi::attr(false))
               >> (( 'b' >> qi::attr(true) )|qi::attr(false))
               >> (( 'k' >> qi::attr(true) )|qi::attr(false))
             ) |
             ( qi::attr(false) >> qi::attr(false) >> qi::attr(false) >> qi::attr(false) >> qi::attr(false) )
           )
         >> ')'
        )
        ;
    r_viewDef.name("view definition");

    r_postproc =

        omit [ postProcFunctionRules [ qi::_a = qi::_1 ] ]
        > qi::lazy(*qi::_a)
            [ phx::bind(&Model::addPostprocActionUnnamed, model_, qi::_1) ];


    r_postproc.name("postprocessing statement");

}

}
}
}
