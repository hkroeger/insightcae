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

#include "booleansubtract.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(BooleanSubtract);
addToFactoryTable(SolidModel, BooleanSubtract, NoParameters);

BooleanSubtract::BooleanSubtract(const NoParameters& nop): SolidModel(nop)
{}


BooleanSubtract::BooleanSubtract(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Cut(m1, m2).Shape())
{
  copyDatums(m1);
  m1.unsetLeaf();
  m2.unsetLeaf();
}

SolidModel operator-(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanSubtract(m1, m2);
}

void BooleanSubtract::insertrule(parser::ISCADParser& ruleset) const
{
//   ruleset.modelstepFunctionRules.add
//   (
//     "",	
//     typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
// 
//     
//       
//     ))
//   );
}

}
}
