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

void ISCADParser::createFeatureExpressions()
{

    r_solidmodel_expression =
        r_solidmodel_term [_val=qi::_1 ]
        >>
        *( '-' >> r_solidmodel_term [ _val = phx::bind(&BooleanSubtract::create, qi::_val, qi::_1) ] )
        ;
    r_solidmodel_expression.name("feature expression");

    r_solidmodel_term =
        r_solidmodel_primary [_val=qi::_1 ]
        >>
        *( '.' > r_identifier
           [ _val = phx::construct<FeaturePtr>(phx::new_<Subfeature>(qi::_val, qi::_1)) ] )
        >>
        -( lit("<<") > r_vectorExpression [ _val = phx::bind(&Transform::create_translate, qi::_val, qi::_1) ] )
        >>
        -( lit("*") > r_scalarExpression [ _val = phx::bind(&Transform::create_scale, qi::_val, qi::_1) ] )
        >>
        *(
            ('|' > r_solidmodel_primary [ _val = phx::bind(&BooleanUnion::create, _val, qi::_1) ] )
            |
            ('&' > (
                 r_solidmodel_primary [ _val = phx::bind(&BooleanIntersection::create, _val, qi::_1) ]
                 |
                 r_datumExpression [ _val = phx::bind(&BooleanIntersection::create_plane, _val, qi::_1) ]
             )
            )
        )
        ;
    r_solidmodel_term.name("feature term");

    r_modelstepFunction %=
        ( current_pos.current_pos >>
          omit[ modelstepFunctionRules [ qi::_a = qi::_1 ] ]
          >> current_pos.current_pos
          >>qi::lazy(*qi::_a) )
        ;

    r_modelstepFunction.name("feature function");

    r_solidmodel_primary =
        ( '*' >> ( r_vertexFeaturesExpression | r_edgeFeaturesExpression | r_faceFeaturesExpression | r_solidFeaturesExpression ) ) 
        [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Feature>(qi::_1)) ]
        |
        r_modelstepFunction
        [ ( _val = phx::at_c<2>(qi::_1),
            phx::bind( &SyntaxElementDirectory::addEntry, syntax_element_locations.get(),
                       phx::construct<SyntaxElementLocation>(phx::at_c<0>(qi::_1), phx::at_c<1>(qi::_1)),
                       phx::at_c<2>(qi::_1) )
          ) ]
        |
        model_->modelstepSymbols()[_val=qi::_1 ]
        |
        ( '(' >> r_solidmodel_expression > ')' )
        [ _val = qi::_1]
        // try identifiers last, since exceptions are generated, if symbols don't exist
        ;
    r_solidmodel_primary.name("feature primary");

    r_solidmodel_propertyAssignment =
        qi::lexeme[ model_->modelstepSymbols() ] [ _a = qi::_1 ]
        >> lit("->") >
        (
            ( lit("density") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setDensity, *_a, qi::_1) ) ]
            |
            ( lit("areaWeight") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setAreaWeight, *_a, qi::_1) ) ]
            |
            ( lit("visresolution") > '=' > r_scalarExpression ) [ lazy( phx::bind(&Feature::setVisResolution, *_a, qi::_1) ) ]
        )
        > ';'
        ;
    r_solidmodel_propertyAssignment.name("feature property assignment");
    
    for (Feature::FactoryTable::const_iterator i = Feature::factories_->begin();
            i != Feature::factories_->end(); i++)
    {
        FeaturePtr sm(i->second->operator()());
        sm->insertrule(*this);
    }
}


}
}
}
