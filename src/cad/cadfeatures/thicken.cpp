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

#include "thicken.h"
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

    
    

defineType(Thicken);
addToFactoryTable(Feature, Thicken);




Thicken::Thicken(): Feature()
{}




Thicken::Thicken(FeaturePtr shell, ScalarPtr thickness, ScalarPtr tol)
: shell_(shell), thickness_(thickness), tol_(tol)
{
    ParameterListHash h(this);
    h+=this->type();
    h+=*shell_;
    h+=thickness_->value();
    h+=tol_->value();
}




FeaturePtr Thicken::create ( FeaturePtr shell, ScalarPtr thickness, ScalarPtr tol )
{
    return FeaturePtr(new Thicken(shell, thickness, tol));
}




void Thicken::build()
{
  ExecTimer t("Thicken::build() ["+featureSymbolName()+"]");

  TopTools_ListOfShape ClosingFaces;
  
  TopoDS_Shape s=*shell_;
  BRepOffset_MakeOffset maker;
  
  double offs=thickness_->value();
  maker.Initialize
  (
      s, offs, 1e-4,
      BRepOffset_Skin, Standard_True, Standard_True, GeomAbs_Arc, Standard_True
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
  
  maker.MakeThickSolid();
//   maker.MakeOffsetShape();
  
  setShape(maker.Shape());
}




void Thicken::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Thicken",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
        >> ruleset.r_solidmodel_expression >> ',' 
        >> ruleset.r_scalarExpression 
        >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst ( Precision::Confusion() )) )
        >> ')' ) 
      [ qi::_val = phx::bind(&Thicken::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Thicken::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Thicken",
            "( <feature:base>, <scalar:t> )",
            "Creates a solid from a shell feature by adding thickness t."
        )
    );
}



}
}
