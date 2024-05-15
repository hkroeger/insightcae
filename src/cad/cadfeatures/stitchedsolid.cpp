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

#include "stitchedsolid.h"
#include "ShapeFix_Solid.hxx"
#include "occinclude.h"
#include "BRepCheck_Shell.hxx"
#include "BRepClass3d_SolidClassifier.hxx"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"
#include "base/translations.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(StitchedSolid);
//addToFactoryTable(Feature, StitchedSolid);
addToStaticFunctionTable(Feature, StitchedSolid, insertrule);
addToStaticFunctionTable(Feature, StitchedSolid, ruleDocumentation);


size_t StitchedSolid::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  for (const FeaturePtr& f: faces_)
  {
      h+=*f;
  }
  h+=tol_->value();
  return h.getHash();
}



StitchedSolid::StitchedSolid(const std::vector<FeaturePtr>& faces, ScalarPtr tol)
: faces_(faces), tol_(tol)
{}

void StitchedSolid::build()
{
  ExecTimer t("StitchedSolid::build() ["+featureSymbolName()+"]");

  BRepBuilderAPI_Sewing sew(tol_->value());
  
//   TopoDS_Compound aRes;
//   BRep_Builder aBuilder;
//   aBuilder.MakeCompound(aRes);

  for (const FeaturePtr& m: faces_)
  {
    sew.Add(*m);
//     aBuilder.Add(aRes, bladeFace_[s]);
  }

  sew.Perform();
  
  TopoDS_Shell sshell = TopoDS::Shell(sew.SewedShape());
  BRepCheck_Shell acheck(sshell);
  
    if (acheck.Closed(Standard_False) != BRepCheck_NoError)
    throw insight::Exception(_("Could not create a closed shell (B)!"));

  if (acheck.Orientation(Standard_False) != BRepCheck_NoError)
    throw insight::Exception(_("Orientation Error!"));
  
  BRepBuilderAPI_MakeSolid solidmaker(sshell);
  
  if (!solidmaker.IsDone())
    throw insight::Exception(_("Creation of solid failed!"));

  auto solid = solidmaker.Solid();

  ShapeFix_Solid fix(solid);
  fix.Perform();
  // BRepClass3d_SolidClassifier classify;
  // classify.Load(solid);
  // classify.PerformInfinitePoint(1.0e-4);
  // if (classify.State()!=TopAbs_IN)
  // {
  //     solid.Reverse();
  // }

  setShape(fix.Shape());
}

void StitchedSolid::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "StitchedSolid",	
    std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' >> (ruleset.r_solidmodel_expression % ',') 
	  >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(1e-3)) ) >> ')' )
      [ qi::_val = phx::bind(
                       &StitchedSolid::create<const std::vector<FeaturePtr>&, ScalarPtr>,
                       qi::_1, qi::_2) ]
      
    )
  );
}

FeatureCmdInfoList StitchedSolid::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "StitchedSolid",

            "( <feature:f1> [, <feature:f2> [, ...] ] [, <scalar:tol> | 0.001 ] )",

          _("Create stitched solid from all faces of the provided features.")
        )
    };
}

}
}
