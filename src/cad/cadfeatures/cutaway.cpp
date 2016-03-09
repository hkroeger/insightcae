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
addToFactoryTable(Feature, Cutaway, NoParameters);

Cutaway::Cutaway(const NoParameters& nop): DerivedFeature(nop)
{}


Cutaway::Cutaway(FeaturePtr model, VectorPtr p0, VectorPtr n)
: DerivedFeature(model), model_(model), p0_(p0), n_(n)
{
}

void Cutaway::build()
{
  ParameterListHash h(this);
  h+=*model_;
  h+=p0_->value();
  h+=n_->value();
  
//   if (!cache.contains(h))
  {

    arma::mat bb=model_->modelBndBox(0.1);
    double L=10.*norm(bb.col(1)-bb.col(0), 2);
    std::cout<<"L="<<L<<std::endl;
    
    arma::mat ex=cross(n_->value(), vec3(1,0,0));
    if (norm(ex,2)<1e-8)
      ex=cross(n_->value(), vec3(0,1,0));
    ex/=norm(ex,2);
    
    arma::mat ey=cross(n_->value(),ex);
    ey/=norm(ey,2);
    
    std::cout<<"Quad"<<std::endl;
  #warning Relocate p0 in plane to somewhere nearer to model center!
    Quad q
    (
      matconst(p0_->value()-0.5*L*(ex+ey)), 
      matconst(L*ex), 
      matconst(L*ey)
    );
    this->setShape(q);
  //   std::cout<<"Airspace"<<std::endl;
    TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(L*n_->value()) );
    
  //   SolidModel(airspace).saveAs("airspace.stp");
    providedSubshapes_["AirSpace"]=FeaturePtr(new Feature(airspace));
    
    std::cout<<"CutSurf"<<std::endl;
    try
    {
      providedSubshapes_["CutSurface"]=
	FeaturePtr
	(
	  new BooleanIntersection
	  (
	    model_, 
	    FeaturePtr(new Feature(TopoDS::Face(q)))
	  )
	);
    }
    catch (...)
    {
      insight::Warning("Could not create cutting surface!");
    }

    std::cout<<"Cut"<<std::endl;
    try
    {
      this->setShape(BRepAlgoAPI_Cut(model_->shape(), airspace));
    }
    catch (...)
    {
      throw insight::Exception("Could not create cut!");
    }
    
//     write(cache.markAsUsed(h));
  }
//   else
//   {
//     read(cache.markAsUsed(h));
//   }
    
}


/** @addtogroup cad_parser
  * @{
  * 
  * @section cutaway Cut away a halfspace
  * Remove everything beyond a plane from a feature
  * 
  * Syntax: 
  * ~~~~
  * Cutaway(<feature expression: model>, <vector: p0>, <vector: n>) : feature
  * ~~~~
  * @}
  */
void Cutaway::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cutaway",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Cutaway>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
