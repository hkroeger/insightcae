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

#include "mirror.h"
#include "datum.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Mirror);
addToFactoryTable(Feature, Mirror, NoParameters);

Mirror::Mirror(const NoParameters& nop): Feature(nop)
{}


Mirror::Mirror(FeaturePtr m1, DatumPtr pl)
: m1_(m1), pl_(pl)
{}

void Mirror::build()
{
  gp_Trsf tr;

  if (!pl_->providesPlanarReference())
    throw insight::Exception("Mirror: planar reference required!");
  
  tr.SetMirror(static_cast<gp_Ax3>(*pl_).Ax2());  
  
  if (m1_->hasExplicitCoG())
  {
    this->setCoGExplicitly( vec3(to_Pnt(m1_->modelCoG()).Transformed(tr)) );
  }
  if (m1_->hasExplicitMass()) setMassExplicitly(m1_->mass());
  
  setShape(BRepBuilderAPI_Transform(m1_->shape(), tr).Shape());
}

void Mirror::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Mirror",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_datumExpression > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Mirror>(qi::_1, qi::_2)) ]
      
    ))
  );
}

}
}
