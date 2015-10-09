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

#include "stitchedshell.h"
#include "occinclude.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(StitchedShell);
addToFactoryTable(SolidModel, StitchedShell, NoParameters);

StitchedShell::StitchedShell(const NoParameters& nop)
: SolidModel(nop)
{
}

StitchedShell::StitchedShell(const FeatureSet& faces, double tol)
: SolidModel()
{
  BRepBuilderAPI_Sewing sew(tol);
  
//   TopoDS_Compound aRes;
//   BRep_Builder aBuilder;
//   aBuilder.MakeCompound(aRes);

  BOOST_FOREACH(const FeatureID& fi, faces)
  {
    sew.Add(faces.model().face(fi));
//     aBuilder.Add(aRes, bladeFace_[s]);
  }

  sew.Perform();
  sew.Dump();
  
  TopoDS_Shell sshell = TopoDS::Shell(sew.SewedShape());
//   BRepCheck_Shell acheck(sshell);
  
  
  setShape(sshell);
}

void StitchedShell::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "StitchedShell",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_faceFeaturesExpression  > ( (',' > qi::double_) | qi::attr(1e-3) ) > ')' )
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<StitchedShell>(*qi::_1, qi::_2)) ]
      
    ))
  );
}

}
}