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

#include "cone.h"
#include "base/boost_include.h"
#include "base/translations.h"

#include "BRepPrimAPI_MakeCone.hxx"

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    
    
defineType(Cone);
//addToFactoryTable(Feature, Cone);
addToStaticFunctionTable(Feature, Cone, insertrule);
addToStaticFunctionTable(Feature, Cone, ruleDocumentation);



size_t Cone::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p1_->value();
  h+=p2_->value();
  h+=D1_->value();
  h+=D2_->value();
  if (di_) h+=di_->value();
  return h.getHash();
}




Cone::Cone(VectorPtr p1, VectorPtr p2, ScalarPtr D1, ScalarPtr D2, ScalarPtr di)
    : p1_(p1), p2_(p2), D1_(D1), D2_(D2), di_(di)
{
}





void Cone::build()
{
    refpoints_["p0"]=p1_->value();
    refpoints_["p1"]=p2_->value();
    refvalues_["D0"]=D1_->value();
    refvalues_["D1"]=D2_->value();

    arma::mat L = p2_->value() - p1_->value();
    double l=arma::norm(L, 2);
    arma::mat el=normalized(L);

    gp_Ax2 ax
        (
            to_Pnt( p1_->value() ),
            gp_Dir( to_Vec ( el ) )
        );

    TopoDS_Shape cone=
        BRepPrimAPI_MakeCone
        (
            ax,
            0.5*D1_->value(),
            0.5*D2_->value(),
            l
        ).Shape();

    if (di_)
    {
        BRepAlgoAPI_Cut cutter(
            cone,
            BRepPrimAPI_MakeCylinder(
                ax,
                0.5*di_->value(),
                l ).Shape() );
        cutter.Build();

        if (!cutter.IsDone())
        {
            throw insight::cad::CADException
                (
                    shared_from_this(),
                    _("could not perform cut operation.")
                    );
        }

        cone=cutter.Shape();
    }

    setShape ( cone );
}




void Cone::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "Cone",
    std::make_shared<parser::ISCADParser::ModelstepRule>(
    ( '(' 
      > ruleset.r_vectorExpression > ','
      > ruleset.r_vectorExpression > ','
      > ruleset.r_scalarExpression > ','
      > ruleset.r_scalarExpression
      > ( ( ',' > ruleset.r_scalarExpression ) | qi::attr(ScalarPtr()) )
      > ')' )
      [ qi::_val = phx::bind(&Cone::create<VectorPtr, VectorPtr, ScalarPtr, ScalarPtr, ScalarPtr>,
                                      qi::_1, qi::_2, qi::_3, qi::_4, qi::_5) ]
      
    )
  );
}




FeatureCmdInfoList Cone::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Cone",
         
            "( <vector:p0>, <vector:p1>, <scalar:D0>, <scalar:D1> )",

          _("Creates a cone between point p0 and p1. At point p0, the diameter is D0 and at p1 it is D1.")
        )
    };
}




}
}
