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

#include "quad.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {



defineType(Quad);
addToFactoryTable(Feature, Quad);




Quad::Quad()
{}




Quad::Quad(VectorPtr p0, VectorPtr L, VectorPtr W, QuadCentering center)
: p0_(p0), L_(L), W_(W), center_(center)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=p0_->value();
  h+=L_->value();
  h+=W_->value();
  h+=boost::fusion::at_c<0>(center_);
  h+=boost::fusion::at_c<1>(center_);
}




FeaturePtr Quad::create(VectorPtr p0, VectorPtr L, VectorPtr W, QuadCentering center)
{
    return FeaturePtr
           (
               new Quad
               (
                   p0, L, W, center
               )
           );
}




void Quad::operator=(const Quad& o)
{
  p0_=o.p0_;
  L_=o.L_;
  W_=o.W_;
  center_=o.center_;
  Feature::operator=(o);
}




void Quad::build()
{
    if ( !cache.contains ( hash() ) ) {
        gp_Pnt
        p1 ( to_Pnt ( p0_->value() ) );

        if ( boost::fusion::at_c<0> ( center_ ) ) {
            p1.Translate ( to_Vec ( -0.5*L_->value() ) );
        }
        if ( boost::fusion::at_c<1> ( center_ ) ) {
            p1.Translate ( to_Vec ( -0.5*W_->value() ) );
        }

        gp_Pnt
        p2=p1.Translated ( to_Vec ( W_->value() ) ),
        p3=p2.Translated ( to_Vec ( L_->value() ) ),
        p4=p1.Translated ( to_Vec ( L_->value() ) )
           ;

        BRepBuilderAPI_MakeWire w;
        w.Add ( BRepBuilderAPI_MakeEdge ( p1, p2 ) );
        w.Add ( BRepBuilderAPI_MakeEdge ( p2, p3 ) );
        w.Add ( BRepBuilderAPI_MakeEdge ( p3, p4 ) );
        w.Add ( BRepBuilderAPI_MakeEdge ( p4, p1 ) );

        refpoints_["c1"]=vec3 ( p1 );
        refpoints_["c2"]=vec3 ( p2 );
        refpoints_["c3"]=vec3 ( p3 );
        refpoints_["c4"]=vec3 ( p4 );

        //   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
        providedSubshapes_["OuterWire"]=FeaturePtr ( new Feature ( w.Wire() ) );

        setShape ( BRepBuilderAPI_MakeFace ( w.Wire() ) );

        cache.insert ( shared_from_this() );
        
    } else {
        this->operator= ( *cache.markAsUsed<Quad> ( hash() ) );
    }
}




Quad::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




void Quad::insertrule(parser::ISCADParser& ruleset) const
{
    ruleset.modelstepFunctionRules.add
    (
        "Quad",
        typename parser::ISCADParser::ModelstepRulePtr ( new typename parser::ISCADParser::ModelstepRule (

                    ( '(' >> ruleset.r_vectorExpression
                      >> ',' >> ruleset.r_vectorExpression
                      >> ',' >> ruleset.r_vectorExpression
                      >> ( ( ',' >> (
                                 ( qi::lit ( "centered" ) >> qi::attr ( true ) >> qi::attr ( true ) )
                                 |
                                 ( qi::lit ( "center" )
                                   >> ( ( 'x' >> qi::attr ( true ) ) |qi::attr ( false ) )
                                   >> ( ( 'y' >> qi::attr ( true ) ) |qi::attr ( false ) )
                                 )
                             ) )
                           |
                           ( qi::attr ( false ) >> qi::attr ( false ) )
                         )
                      >> ')' )
                    [ qi::_val = phx::bind ( &Quad::create, qi::_1, qi::_2, qi::_3, qi::_4 ) ]

                ) )
    );
}




FeatureCmdInfoList Quad::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Quad",
            "( <vector:p0>, <vector:Lx>, <vector:Ly> [, centered | (center [x][y]) ] )",
            "Creates a quad face. The quad is located at point p0 and direction and edge lengths are defined by the vector Lx, Ly.\n"
            "Optionally, the edges are centered around p0. Either all directions (option centered) or only selected directions (option center [x][y] where x,y is associated with L1, L2 respectively)."
        )
    );
}




}
}
