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
 */

#include "shoulder.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(Shoulder);
addToFactoryTable(Feature, Shoulder, NoParameters);

Shoulder::Shoulder(const NoParameters&)
{}


Shoulder::Shoulder(VectorPtr p0, VectorPtr dir, ScalarPtr d, ScalarPtr Dmax)
: p0_(p0), dir_(dir), d_(d), Dmax_(Dmax)
{}

void Shoulder::build()
{
  TopoDS_Face xsec_o=BRepBuilderAPI_MakeFace
  (
    BRepBuilderAPI_MakeWire
    (
      BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2( to_Pnt(p0_->value()), to_Vec(dir_->value()) ), 0.5*Dmax_->value() ))
    )
  );
  TopoDS_Face xsec=BRepBuilderAPI_MakeFace
  (
    xsec_o,
    BRepBuilderAPI_MakeWire
    (
      BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2( to_Pnt(p0_->value()), to_Vec(dir_->value()) ), 0.5*d_->value() ))
    )
  );
  
  setShape
  (
    BRepPrimAPI_MakePrism
    ( 
      xsec, 
//       gp_Dir(to_Vec(dir)), false 
      to_Vec(dir_->value())*1e6
    )
  ); // semi-infinite prism
}

void Shoulder::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Shoulder",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > 
			    ',' > ruleset.r_scalarExpression > ((',' > ruleset.r_scalarExpression)|qi::attr(scalarconst(1e6))) > ')' )
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Shoulder>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}

}
}
