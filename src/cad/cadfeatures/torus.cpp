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

#include "torus.h"
#include "base/boost_include.h"

#include "BRepPrimAPI_MakeTorus.hxx"

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(Torus);
addToFactoryTable(Feature, Torus, NoParameters);

Torus::Torus(const NoParameters&)
{}

Torus::Torus(VectorPtr p0, VectorPtr axisTimesD, ScalarPtr d)
: p0_(p0), axisTimesD_(axisTimesD), d_(d)
{}

void Torus::build()
{
  double D=arma::norm(axisTimesD_->value(), 2);
  arma::mat axis=axisTimesD_->value()/D;
  
  refpoints_["p0"]=p0_->value();
  refvectors_["axis"]=axis;
  refvalues_["D"]=D;
  refvalues_["d"]=d_->value();
  
  TopoDS_Shape tor=
    BRepPrimAPI_MakeTorus
    (
      gp_Ax2
      (
	to_Pnt(p0_->value()), 
	gp_Dir(to_Vec(axis))
      ),
      0.5*D, 
      0.5*d_->value()
    ).Shape();
    
  setShape(tor);
}

void Torus::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Torus",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_scalarExpression 
      >> ')' ) 
     [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Torus>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}