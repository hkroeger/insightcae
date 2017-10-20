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

void ISCADParser::createDatumExpressions()
{
    r_datumExpression =
// 	  qi::lexeme[model_->datumSymbols()]
// 	    [ _val =  phx::bind(&Model::lookupDatum, model_, qi::_1) ]
      (
        model_->datumSymbols()[ qi::_val = qi::_1 ]
        |
        ( lit("Plane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2)) ]
        |
        ( lit("SPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2, qi::_3)) ]
        |
        ( lit("TPlane") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<DatumPlane>(qi::_1, qi::_2, qi::_3, true)) ]
        |
        ( lit("RefPt") >> '(' >> r_vectorExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<ExplicitDatumPoint>(qi::_1)) ]
        |
        ( lit("RefAxis") >> '(' >> r_vectorExpression >> ',' >> r_vectorExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<ExplicitDatumAxis>(qi::_1, qi::_2)) ]
        |
        ( lit("xsec_axpl") >> '(' >> r_datumExpression >> ',' >> r_datumExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<XsecAxisPlane>(qi::_1, qi::_2)) ]
        |
        ( lit("xsec_plpl") >> '(' >> r_datumExpression >> ',' >> r_datumExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<XsecPlanePlane>(qi::_1, qi::_2)) ]
        |
        ( lit("xsec_ppp") >> '(' >> r_datumExpression >> ',' >> r_datumExpression >> ',' >> r_datumExpression >> ')' )
        [ _val = construct<DatumPtr>(new_<XsecAxisPlane>(
                                         construct<DatumPtr>(new_<XsecPlanePlane>(qi::_1, qi::_2)),
                                         qi::_3)) ]
        |
//           ( lit("datum") > '(' > r_solidmodel_expression > '%' > r_identifier > ')' )
        ( ( r_solidmodel_expression >> '%' ) >> r_identifier  )
        [ _val = phx::construct<DatumPtr>(new_<ProvidedDatum>(qi::_1, qi::_2)) ]
      )
      >> -( lit("<<") >> r_vectorExpression ) [ _val = construct<DatumPtr>(new_<TransformedDatum>(_val, qi::_1)) ];

    r_datumExpression.name("datum expression");
}


}
}
}
