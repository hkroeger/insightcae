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




Quad::Quad(VectorPtr p0, VectorPtr L, VectorPtr W, ScalarPtr t, QuadCentering center)
: p0_(p0), L_(L), W_(W), t_(t), center_(center)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=p0_->value();
  h+=L_->value();
  h+=W_->value();
  if (t_) h+=t_->value();
  h+=boost::fusion::at_c<0>(center_);
  h+=boost::fusion::at_c<1>(center_);
}




FeaturePtr Quad::create(VectorPtr p0, VectorPtr L, VectorPtr W, ScalarPtr t, QuadCentering center)
{
    return FeaturePtr
           (
               new Quad
               (
                   p0, L, W, t, center
               )
           );
}




void Quad::operator=(const Quad& o)
{
  p0_=o.p0_;
  L_=o.L_;
  W_=o.W_;
  t_=o.t_;
  center_=o.center_;
  Feature::operator=(o);
}




void Quad::build()
{
    if ( !cache.contains ( hash() ) ) 
    {
        double L=arma::norm(L_->value(), 2);
        double W=arma::norm(W_->value(), 2);
        
        refvalues_["W"]=W;
        refvalues_["L"]=L;
        
        refvectors_["L"]=L_->value();
        refvectors_["W"]=W_->value();
        
        if (L<1e-10)
            throw insight::Exception(str(format("Invalid parameter: Length of quad needs to be larger than 0 (is %g)!")%L));
        if (W<1e-10)
            throw insight::Exception(str(format("Invalid parameter: Width of quad needs to be larger than 0 (is %g)!")%W));
        
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

        TopoDS_Shape s = BRepBuilderAPI_MakeFace ( w.Wire() );
        
        if (t_)
        {
            double maxt=std::min(L,W)/2.;
            double t=t_->value();
            refvalues_["t"]=t;
            
            if (t<1e-10)
                throw insight::Exception(str(format("Invalid parameter: Wall thickness of quad needs to be larger than 0 (is %g)!")%t));
            if (t>maxt)
                throw insight::Exception(str(format("Invalid parameter: Wall thickness of quad is larger than half width or half height (%g > %g)!")%t%maxt));
            
            gp_Pnt
                p1 ( to_Pnt ( p0_->value() ) );
                
            p1.Translate ( to_Vec ( L_->value()*(t/L) + W_->value()*(t/W) ) );

            if ( boost::fusion::at_c<0> ( center_ ) ) {
                p1.Translate ( to_Vec ( -0.5*L_->value() ) );
            }
            if ( boost::fusion::at_c<1> ( center_ ) ) {
                p1.Translate ( to_Vec ( -0.5*W_->value() ) );
            }

            gp_Pnt
            p2=p1.Translated ( to_Vec ( W_->value()*((W-2*t)/W) ) ),
            p3=p2.Translated ( to_Vec ( L_->value()*((L-2*t)/L) ) ),
            p4=p1.Translated ( to_Vec ( L_->value()*((L-2*t)/L) ) )
            ;

            BRepBuilderAPI_MakeWire w;
            w.Add ( BRepBuilderAPI_MakeEdge ( p1, p2 ) );
            w.Add ( BRepBuilderAPI_MakeEdge ( p2, p3 ) );
            w.Add ( BRepBuilderAPI_MakeEdge ( p3, p4 ) );
            w.Add ( BRepBuilderAPI_MakeEdge ( p4, p1 ) );
            
            s = BRepAlgoAPI_Cut( s,  BRepBuilderAPI_MakeFace ( w.Wire() ) );
        }
        
        setShape ( s );

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
                      >> ( ( ',' >> ruleset.r_scalarExpression ) | qi::attr ( ScalarPtr() ) )
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
                    [ qi::_val = phx::bind ( &Quad::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5 ) ]

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
            "( <vector:p0>, <vector:Lx>, <vector:Ly> [, <thickness>] [, centered | (center [x][y]) ] )",
            "Creates a quad face. The quad is located at point p0 and direction and edge lengths are defined by the vector Lx, Ly.\n"
            "Optionally, the edges are centered around p0. Either all directions (option centered) or only selected directions (option center [x][y] where x,y is associated with L1, L2 respectively)."
        )
    );
}




}
}
