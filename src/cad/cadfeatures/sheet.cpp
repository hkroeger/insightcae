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

#include "sheet.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"
#include "base/translations.h"

// #include "BRepOffsetAPI_MakeThickSolid.hxx"
#include "BRepOffset_MakeOffset.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;




namespace insight {
namespace cad {

    
    

defineType(Sheet);
//addToFactoryTable(Feature, Thicken);
addToStaticFunctionTable(Feature, Sheet, insertrule);
addToStaticFunctionTable(Feature, Sheet, ruleDocumentation);


size_t Sheet::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*shell_;
  h+=thickness_->value();
  h+=tol_->value();
  return h.getHash();
}






Sheet::Sheet(FeaturePtr shell, ScalarPtr thickness, ScalarPtr tol)
: shell_(shell), thickness_(thickness), tol_(tol)
{}






void Sheet::build()
{
  ExecTimer t("Sheet::build() ["+featureSymbolName()+"]");

  TopTools_ListOfShape ClosingFaces;
  
  TopoDS_Shape s=*shell_;
  
  double offs=thickness_->value();

  providedSubshapes_["shell"]=shell_;

  BRepOffset_MakeOffset maker;
  maker.Initialize
  (
      s, offs, Precision::Confusion(),
      BRepOffset_Skin, Standard_True, Standard_True, GeomAbs_Arc, Standard_True
  );
  for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
  {
      maker.SetOffsetOnFace(TopoDS::Face(ex.Current()), offs);
  }

  
  maker.MakeThickSolid();
  
  TopoDS_Shape res=maker.Shape();
  ShapeFix_Solid FixShape;

  FixShape.Init(TopoDS::Solid(res));
  FixShape.Perform();

  setShape(FixShape.Solid());
}




void Sheet::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "Sheet",
    std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' 
        >> ruleset.r_solidmodel_expression >> ',' 
        >> ruleset.r_scalarExpression 
        >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst ( Precision::Confusion() )) )
        >> ')' ) 
      [ qi::_val = phx::bind(
                       &Sheet::create<FeaturePtr, ScalarPtr, ScalarPtr>,
                       qi::_1, qi::_2, qi::_3) ]
      
    )
  );
}




FeatureCmdInfoList Sheet::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Sheet",
            "( <feature:base>, <scalar:t> )",
          _("Creates a solid from a shell feature by adding thickness t.")
        )
    };
}



}
}
