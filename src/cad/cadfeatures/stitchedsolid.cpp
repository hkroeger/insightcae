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
#include "occinclude.h"
#include "BRepCheck_Shell.hxx"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/tools.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(StitchedSolid);
addToFactoryTable(Feature, StitchedSolid);

StitchedSolid::StitchedSolid()
{}


StitchedSolid::StitchedSolid(const std::vector<FeaturePtr>& faces, ScalarPtr tol)
: faces_(faces), tol_(tol)
{
    ParameterListHash h(this);
    h+=this->type();
    BOOST_FOREACH(const FeaturePtr& f, faces_)
    {
        h+=*f;
    }
    h+=tol_->value();
}

void StitchedSolid::build()
{
  ExecTimer t("StitchedSolid::build() ["+featureSymbolName()+"]");

  BRepBuilderAPI_Sewing sew(tol_->value());
  
//   TopoDS_Compound aRes;
//   BRep_Builder aBuilder;
//   aBuilder.MakeCompound(aRes);

  BOOST_FOREACH(const FeaturePtr& m, faces_)
  {
    sew.Add(*m);
//     aBuilder.Add(aRes, bladeFace_[s]);
  }

  sew.Perform();
  sew.Dump();
  
  TopoDS_Shell sshell = TopoDS::Shell(sew.SewedShape());
  BRepCheck_Shell acheck(sshell);
  
    if (acheck.Closed(Standard_False) != BRepCheck_NoError)
    throw insight::Exception("Could not create a closed shell (B)!");

  if (acheck.Orientation(Standard_False) != BRepCheck_NoError)
    throw insight::Exception("Orientation Error!");
  
  BRepBuilderAPI_MakeSolid solidmaker(sshell);
  
  if (!solidmaker.IsDone())
    throw insight::Exception("Creation of solid failed!");

  setShape(solidmaker.Solid());
}

void StitchedSolid::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "StitchedSolid",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> (ruleset.r_solidmodel_expression % ',') 
	  >> ( (',' >> ruleset.r_scalarExpression) | qi::attr(scalarconst(1e-3)) ) >> ')' )
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<StitchedSolid>(qi::_1, qi::_2)) ]
      
    ))
  );
}


}
}
