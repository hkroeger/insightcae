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

#include "offset.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"

// #include "BRepOffsetAPI_MakeThickSolid.hxx"
#include "BRepOffset_MakeOffset.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;




namespace insight {
namespace cad {




defineType(Offset);
addToFactoryTable(Feature, Offset);



size_t Offset::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*shell_;
  h+=thickness_->value();
  h+=tol_->value();
  return h.getHash();
}



Offset::Offset(): Feature()
{}




Offset::Offset(FeaturePtr shell, ScalarPtr thickness, ScalarPtr tol)
: shell_(shell), thickness_(thickness), tol_(tol)
{}




FeaturePtr Offset::create ( FeaturePtr shell, ScalarPtr thickness, ScalarPtr tol )
{
    return FeaturePtr(new Offset(shell, thickness, tol));
}




void Offset::build()
{
  ExecTimer t("Offset::build() ["+featureSymbolName()+"]");

  TopTools_ListOfShape ClosingFaces;

  TopoDS_Shape s=*shell_;
  BRepOffset_MakeOffset maker;

  double offs=thickness_->value();
  maker.Initialize
  (
      s, offs, Precision::Confusion(),
      BRepOffset_Skin, Standard_True, Standard_True, GeomAbs_Arc, Standard_False
  );
  for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
  {
      maker.SetOffsetOnFace(TopoDS::Face(ex.Current()), offs);
  }
//   BRepOffsetAPI_MakeThickSolid maker
//   (
//     *shell_, ClosingFaces, thickness_->value(),
//     Precision::Confusion(),
//     BRepOffset_Skin,
//     Standard_True,
//     Standard_False,
//     GeomAbs_Arc
//   );

//  maker.MakeThickSolid();
  maker.MakeOffsetShape();

  setShape(maker.Shape());
}




void Offset::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Offset",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '('
        >> ruleset.r_solidmodel_expression >> ','
        >> ruleset.r_scalarExpression
        >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst ( Precision::Confusion() )) )
        >> ')' )
      [ qi::_val = phx::bind(&Offset::create, qi::_1, qi::_2, qi::_3) ]

    ))
  );
}




FeatureCmdInfoList Offset::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Offset",
            "( <feature:base>, <scalar:t> )",
            "Creates an offset surface from a shell feature by displacing the surfaces in normal direction by distance t."
        )
    );
}



}
}
