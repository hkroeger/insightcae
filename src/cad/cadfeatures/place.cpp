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

#include "place.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Place);
addToFactoryTable(Feature, Place, NoParameters);

Place::Place(const NoParameters& nop): Feature(nop)
{}


Place::Place(FeaturePtr m, const gp_Ax2& cs)
: m_(m) 
{
  trsf_.reset(new gp_Trsf);
  trsf_->SetTransformation(gp_Ax3(cs));
  
  trsf_->Invert();
//   makePlacement(m, tr.Inverted());
}


Place::Place(FeaturePtr m, VectorPtr p0, VectorPtr ex, VectorPtr ez)
: m_(m), p0_(p0), ex_(ex), ez_(ez)
{}

void Place::build()
{
  if (!trsf_)
  {
    trsf_.reset(new gp_Trsf);
    trsf_->SetTransformation(gp_Ax3(to_Pnt(p0_->value()), to_Vec(ez_->value()), to_Vec(ex_->value())));
    trsf_->Invert();
  //   makePlacement(m, tr.Inverted());
  }
 
  setShape(BRepBuilderAPI_Transform(m_->shape(), *trsf_).Shape());
  
  copyDatumsTransformed(*m_, *trsf_);
}

void Place::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Place",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > 
	  ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Place>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}


}
}
