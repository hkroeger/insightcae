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

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {


    
    
defineType(Box);
addToFactoryTable(Feature, Box);




Box::Box()
{}



  
Box::Box
(
  VectorPtr p0, 
  VectorPtr L1, 
  VectorPtr L2, 
  VectorPtr L3,
  BoxCentering center
)
: p0_(p0), L1_(L1), L2_(L2), L3_(L3), center_(center)
{}



FeaturePtr Box::create
(
    VectorPtr p0,
    VectorPtr L1,
    VectorPtr L2,
    VectorPtr L3,
    BoxCentering center
)
{
    return FeaturePtr(new Box(p0, L1, L2, L3, center));
}




void Box::build()
{ 
    arma::mat p0=p0_->value();
    if (boost::fusion::at_c<0>(center_))  p0-=0.5*+L1_->value();
    if (boost::fusion::at_c<1>(center_))  p0-=0.5*+L2_->value();
    if (boost::fusion::at_c<2>(center_))  p0-=0.5*+L3_->value();

    refpoints_["p0"]=p0;

    refvalues_["L1"]=arma::norm(L1_->value(), 2);
    refvalues_["L2"]=arma::norm(L2_->value(), 2);
    refvalues_["L3"]=arma::norm(L3_->value(), 2);

    refvectors_["e1"]=L1_->value()/arma::norm(L1_->value(), 2);
    refvectors_["e2"]=L2_->value()/arma::norm(L2_->value(), 2);
    refvectors_["e3"]=L3_->value()/arma::norm(L3_->value(), 2);

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
}




void Box::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Box",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
        > ruleset.r_vectorExpression > ',' 
        > ruleset.r_vectorExpression > ',' 
        > ruleset.r_vectorExpression > ',' 
        > ruleset.r_vectorExpression 
        > ( ( ',' >> (
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
        > ')' ) 
      [ qi::_val = phx::bind(&Box::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5) ]
      
    ))
  );
}



FeatureCmdInfoList Box::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Box",
         
            "( <vector:p0>, <vector:L1>, <vector:L2>, <vector:L3>\n"
            "[, centered | ( center [x][y][z] ) ] )",
         
            "Creates a box volume. The box is located at point p0 and direction and edge lengths are defined by the vector L1, L2, L3.\n"
            "Optionally, the edges are centered around p0. Either all directions (option centered) or only selected directions (option center [x][y][z] where x,y,z is associated with L1, L2, and L3 respectively)."
        )
    );
}



}
}
