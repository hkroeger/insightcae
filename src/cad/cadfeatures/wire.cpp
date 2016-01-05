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

#include "wire.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

defineType(Wire);
addToFactoryTable(Feature, Wire, NoParameters);

Wire::Wire(const NoParameters& nop): Feature(nop)
{

}


Wire::Wire(FeatureSetPtr edges)
: edges_(edges)
{}

void Wire::build()
{
  std::cout<<"create wire: start"<<std::endl;
  
  TopTools_ListOfShape ee;
  BOOST_FOREACH(const FeatureID& fi, edges_->data())
  {
    std::cout<<"edge #"<<fi<<std::endl;
    ee.Append(edges_->model()->edge(fi));
  }
  BRepBuilderAPI_MakeWire wb;
  wb.Add(ee);
  setShape(wb.Wire());
  
  std::cout<<"create wire: finished"<<std::endl;
}

void Wire::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Wire",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_edgeFeaturesExpression >> ')' ) 
	  [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Wire>(qi::_1)) ]
      
    ))
  );
}

bool Wire::isSingleClosedWire() const
{
  return TopoDS::Wire(shape()).Closed();
}

bool Wire::isSingleOpenWire() const
{
  return !isSingleClosedWire();
}

}
}