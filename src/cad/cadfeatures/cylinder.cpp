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

#include "cylinder.h"
#include "base/boost_include.h"
#include "base/tools.h"
#include "datum.h"
#include "base/translations.h"

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    

defineType ( Cylinder );
//addToFactoryTable ( Feature, Cylinder );
addToStaticFunctionTable(Feature, Cylinder, insertrule);
addToStaticFunctionTable(Feature, Cylinder, ruleDocumentation);



size_t Cylinder::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p2isAxis_;
  h+=p1_->value();
  h+=p2_->value();
  h+=D_->value();
  if (Di_) h+=Di_->value();
  h+=centered_;
  return h.getHash();
}





Cylinder::Cylinder ( VectorPtr p1, VectorPtr p2, ScalarPtr D, bool p2isAxis, bool centered )
    : p2isAxis_ ( p2isAxis ), p1_ ( p1 ), p2_ ( p2 ), D_ ( D ), centered_ ( centered )
{
}




Cylinder::Cylinder ( VectorPtr p1, VectorPtr p2, ScalarPtr Da, ScalarPtr Di, bool p2isAxis, bool centered )
    : p2isAxis_ ( p2isAxis ), p1_ ( p1 ), p2_ ( p2 ), D_ ( Da ), Di_ ( Di ), centered_ ( centered )
{
}





void Cylinder::build()
{
  ExecTimer t("Cylinder::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
  {
    arma::mat p1, p2;

    p1=p1_->value();
    if ( p2isAxis_ ) {
        p2=p1_->value()+p2_->value();
    } else {
        p2=p2_->value();
    }

    double L=arma::norm ( p2-p1,2 );
    arma::mat ax= ( p2-p1 ) /L;
    if ( centered_ ) {
        p1 -= 0.5*L*ax;
        p2 -= 0.5*L*ax;
    }

    refpoints_["p0"]=p1;
    refpoints_["p1"]=p2;
    refvalues_["Da"]=D_->value();
    refvectors_["axis"]=(p2-p1)/L;

    TopoDS_Shape cyl=
        BRepPrimAPI_MakeCylinder
        (
            gp_Ax2
            (
                to_Pnt ( p1 ), //gp_Pnt( p1(0), p1(1), p1(2) ),
                gp_Dir ( to_Vec ( ax ) ) //p2(0) - p1_->value()(0), p2(1) - p1_->value()(1), p2(2) - p1_->value()(2))
            ),
            0.5*D_->value(),
            L
        ).Shape();

    if ( Di_ ) {
        refvalues_["Di"]=Di_->value();
        cyl=BRepAlgoAPI_Cut
            (

                cyl,

                BRepPrimAPI_MakeCylinder
                (
                    gp_Ax2
                    (
                        to_Pnt ( p1 ),
                        gp_Dir ( to_Vec(ax) )
                    ),
                    0.5*Di_->value(),
                    L
                ).Shape()

            );
    }

    providedDatums_["axis"]=DatumPtr ( new ExplicitDatumAxis ( p1_, VectorPtr ( new SubtractedVector ( p2_, p1_ ) ) ) );

    setShape ( cyl );
    cache.insert(shared_from_this());
  }
  else
  {
      this->operator=(*cache.markAsUsed<Cylinder>(hash()));
  }
}




void Cylinder::insertrule ( parser::ISCADParser& ruleset )
{
    ruleset.modelstepFunctionRules.add
    (
        "Cylinder",
        std::make_shared<parser::ISCADParser::ModelstepRule>(

                    ( '('
                      >> ruleset.r_vectorExpression >> ','
                      >> ( ( qi::lit ( "ax" ) >> qi::attr ( true ) ) | qi::attr ( false ) )
                      >> ruleset.r_vectorExpression >> ','
                      >> ruleset.r_scalarExpression
                      >> ( ( ',' >> ruleset.r_scalarExpression ) | qi::attr ( ScalarPtr() ) )
                      >> ( ( ',' >> qi::lit ( "centered" ) >> qi::attr ( true ) ) | qi::attr ( false ) )
                      >> ')' )
                    [ qi::_val = phx::bind (
                       &Cylinder::create<VectorPtr, VectorPtr, ScalarPtr, ScalarPtr, bool, bool>,
                       qi::_1, qi::_3, qi::_4, qi::_5, qi::_2, qi::_6 ) ]

                )
    );
}




FeatureCmdInfoList Cylinder::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Cylinder",
         
            "( <vector:p0>, [ax] <vector:p1_or_axis>, <scalar:Da> [, <scalar:Di] [, centered] )",
         
            _("Creates a cylinder based on point p0. The cylinder extends up to point p1_or_axis or, if the keyword ax is given, along the length vector p1_or_axis."
            " The outer diameter of the cylinder is Da."
            " If an inner diameter Di is given, a hollow cylinder is created."
              " The cylinder is centered with respect to p0, if the keyword centered is supplied.")
        )
    };
}



}
}
