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

#include "extrusion.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"
#include "base/translations.h"
#include "cadfeature.h"
#include "cadfeatures/importsolidmodel.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Extrusion);
addToStaticFunctionTable(Feature, Extrusion, insertrule);
addToStaticFunctionTable(Feature, Extrusion, ruleDocumentation);



size_t Extrusion::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*sk_;

  if (const auto* Lvec = boost::get<VectorPtr>(&L_))
  {
      h+=(*Lvec)->value();
  }
  else if (const auto* Lsc = boost::get<ScalarPtr>(&L_))
  {
      h+=(*Lsc)->value();
  }

  h+=centered_;
  return h.getHash();
}




Extrusion::Extrusion(FeaturePtr sk, ScalarPtr L, bool centered)
    : sk_(sk), L_(L), centered_(centered)
{
}



Extrusion::Extrusion(FeaturePtr sk, VectorPtr L, bool centered)
: sk_(sk), L_(L), centered_(centered)
{
}





void Extrusion::build()
{
    ExecTimer t("Extrusion::build() ["+featureSymbolName()+"]");

    arma::mat L;
    if (const auto* Lvec = boost::get<VectorPtr>(&L_))
    {
      L=(*Lvec)->value();
    }
    else if (const auto* Lsc = boost::get<ScalarPtr>(&L_))
    {
      L = sk_->averageFaceNormal()*(*Lsc)->value();
    }

    bool extrusionOfEdges =
        sk_->topologicalProperties().onlyEdges();


    TopoDS_Shape toExtr;
    if ( !centered_ )
    {
        toExtr=sk_->shape();
    }
    else
    {
        gp_Trsf trsf;
        trsf.SetTranslation ( to_Vec(-0.5*L) );
        toExtr=BRepBuilderAPI_Transform ( sk_->shape(), trsf ).Shape();
    }


    BRepPrimAPI_MakePrism mkp ( toExtr, to_Vec(L ) );
    auto f=Import::create ( mkp.FirstShape() );
    auto b=Import::create ( mkp.LastShape() );

    if (extrusionOfEdges)
    {
        providedSubshapes_["frontEdge"]=f;
        providedSubshapes_["backEdge"]=b;
        providedFeatureSets_["frontEdge"]=makeEdgeFeatureSet(
            shared_from_this(), "isSame(%0)", {f->allEdges()});
        providedFeatureSets_["backEdge"]=makeEdgeFeatureSet(
            shared_from_this(), "isSame(%0)", {b->allEdges()});
    }
    else
    {
        providedSubshapes_["frontFace"]=f;
        providedSubshapes_["backFace"]=b;
        providedFeatureSets_["frontFace"]=makeFaceFeatureSet(
            shared_from_this(), "isSame(%0)", {f->allFaces()});
        providedFeatureSets_["backFace"]=makeFaceFeatureSet(
            shared_from_this(), "isSame(%0)", {b->allFaces()});
    }

    setShape ( mkp.Shape() );


    copyDatums ( *sk_ );
}




void Extrusion::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "Extrusion",
    std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_vectorExpression
      >> ( (  ',' >> qi::lit("centered") >> qi::attr(true) ) | qi::attr(false) ) 
      >> ')' )
      [ qi::_val = phx::bind(
                         &Extrusion::create<FeaturePtr, VectorPtr, bool>,
                         qi::_1, qi::_2, qi::_3) ]
    |
    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_scalarExpression
      >> ( (  ',' >> qi::lit("centered") >> qi::attr(true) ) | qi::attr(false) )
      >> ')' )
      [ qi::_val = phx::bind(
                         &Extrusion::create<FeaturePtr, ScalarPtr, bool>,
                         qi::_1, qi::_2, qi::_3) ]

    )
  );
}




FeatureCmdInfoList Extrusion::ruleDocumentation()
{
  return {
        FeatureCmdInfo
        (
            "Extrusion",
         
            "( <feature:sec>, <vector:L> [, centered] )",
         
            _("Creates an extrusion of the planar feature sec. The direction and length of the extrusion is given by the vector L."
            " If the keyword centered is given, the extrusion is centered around the base feature.")
        )
    };
}



}
}
