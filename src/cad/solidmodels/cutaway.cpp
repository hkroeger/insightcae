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

#include "cutaway.h"
#include "quad.h"
#include "booleanintersection.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Cutaway);
addToFactoryTable(SolidModel, Cutaway, NoParameters);

Cutaway::Cutaway(const NoParameters& nop): SolidModel(nop)
{}


Cutaway::Cutaway(const SolidModel& model, const arma::mat& p0, const arma::mat& n)
{
  arma::mat bb=model.modelBndBox(0.1);
  double L=10.*norm(bb.col(1)-bb.col(0), 2);
  std::cout<<"L="<<L<<std::endl;
  
  arma::mat ex=cross(n, vec3(1,0,0));
  if (norm(ex,2)<1e-8)
    ex=cross(n, vec3(0,1,0));
  ex/=norm(ex,2);
  
  arma::mat ey=cross(n,ex);
  ey/=norm(ey,2);
  
  std::cout<<"Quad"<<std::endl;
#warning Relocate p0 in plane to somewhere nearer to model center!
  Quad q(p0-0.5*L*(ex+ey), L*ex, L*ey);
  this->setShape(q);
//   std::cout<<"Airspace"<<std::endl;
  TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(L*n) );
  
//   SolidModel(airspace).saveAs("airspace.stp");
  providedSubshapes_.add("AirSpace", SolidModelPtr(new SolidModel(airspace)));
  
  std::cout<<"CutSurf"<<std::endl;
  try
  {
    providedSubshapes_.add("CutSurface", SolidModelPtr(new BooleanIntersection(model, TopoDS::Face(q))));
  }
  catch (...)
  {
    insight::Warning("Could not create cutting surface!");
  }

  std::cout<<"Cut"<<std::endl;
  try
  {
    this->setShape(BRepAlgoAPI_Cut(model, airspace));
  }
  catch (...)
  {
    throw insight::Exception("Could not create cut!");
  }
}

void Cutaway::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cutaway",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Cutaway>(*qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
