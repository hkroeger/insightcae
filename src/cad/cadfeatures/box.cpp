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

#include "box.h"
#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/translations.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {


    
    
defineType(Box);
//addToFactoryTable(Feature, Box);
addToStaticFunctionTable(Feature, Box, insertrule);
addToStaticFunctionTable(Feature, Box, ruleDocumentation);

size_t Box::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p0_->value();
  h+=L1_->value();
  h+=L2_->value();
  h+=L3_->value();
  h+=boost::fusion::at_c<0>(center_);
  h+=boost::fusion::at_c<1>(center_);
  h+=boost::fusion::at_c<2>(center_);
  return h.getHash();
}



  
Box::Box
(
  VectorPtr p0, 
  VectorPtr L1, 
  VectorPtr L2, 
  VectorPtr L3,
  BoxCentering center
)
: p0_(p0), L1_(L1), L2_(L2), L3_(L3), center_(center)
{
}






void Box::build()
{ 
  ExecTimer t("Box::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
  {
    arma::mat p0=p0_->value();
    if (boost::fusion::at_c<0>(center_))  p0-=0.5*+L1_->value();
    if (boost::fusion::at_c<1>(center_))  p0-=0.5*+L2_->value();
    if (boost::fusion::at_c<2>(center_))  p0-=0.5*+L3_->value();

    refpoints_["p0"]=p0;
    refpoints_["p1"]=p0+L1_->value()+L2_->value()+L3_->value();

    refvalues_["L1"]=arma::norm(L1_->value(), 2);
    refvalues_["L2"]=arma::norm(L2_->value(), 2);
    refvalues_["L3"]=arma::norm(L3_->value(), 2);

    refvectors_["e1"]=L1_->value()/arma::norm(L1_->value(), 2);
    refvectors_["e2"]=L2_->value()/arma::norm(L2_->value(), 2);
    refvectors_["e3"]=L3_->value()/arma::norm(L3_->value(), 2);

    refvectors_["L1"]=L1_->value();
    refvectors_["L2"]=L2_->value();
    refvectors_["L3"]=L3_->value();

    Handle_Geom_Plane pln=GC_MakePlane(to_Pnt(p0), to_Pnt(p0+L1_->value()), to_Pnt(p0+L2_->value())).Value();
    TopoDS_Shape box=
        BRepPrimAPI_MakePrism
        (
            BRepBuilderAPI_MakeFace
            (
                pln,
                BRepBuilderAPI_MakePolygon
                (
                    to_Pnt(p0),
                    to_Pnt(p0+L1_->value()),
                    to_Pnt(p0+L1_->value()+L2_->value()),
                    to_Pnt(p0+L2_->value()),
                    true
                ).Wire()
            ).Face(),
            to_Vec(L3_->value())
        ).Shape();

    setShape(box);

    cache.insert(shared_from_this());
  }
  else
  {
      this->operator=(*cache.markAsUsed<Box>(hash()));
  }
}




void Box::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "Box",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
        >> ruleset.r_vectorExpression >> ',' 
        >> ruleset.r_vectorExpression >> ',' 
        >> ruleset.r_vectorExpression >> ',' 
        >> ruleset.r_vectorExpression 
        >> ( ( ',' >> (
            (  qi::lit("centered") >> qi::attr(true) >> qi::attr(true) >> qi::attr(true) )
            |
            (  qi::lit("center") 
            >> (( 'x' >> qi::attr(true) )|qi::attr(false))
            >> (( 'y' >> qi::attr(true) )|qi::attr(false))
            >> (( 'z' >> qi::attr(true) )|qi::attr(false))
            )
            ) )
            |
            ( qi::attr(false) >> qi::attr(false) >> qi::attr(false) )
          )
        >> ')' ) 
      [ qi::_val = phx::bind(
                       &Box::create<VectorPtr, VectorPtr, VectorPtr, VectorPtr, BoxCentering>,
                       qi::_1, qi::_2, qi::_3, qi::_4, qi::_5) ]
      
    ))
  );
}



FeatureCmdInfoList Box::ruleDocumentation()
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Box",
         
            "( <vector:p0>, <vector:L1>, <vector:L2>, <vector:L3>\n"
            "[, centered | ( center [x][y][z] ) ] )",
         
            _("Creates a box volume. The box is located at point p0 and edge directions and lengths are defined by the vectors L1, L2, L3.\n"
            "By default, p0 is a corner and the other corners are found by translating p0 along combinations of L1, L2, L3."
            " Optionally, the edges are centered around p0."
            " Either all directions (option centered) or only selected directions (option center [x][y][z] where x,y,z is associated with L1, L2, and L3 respectively).")
        )
    );
}



}
}
