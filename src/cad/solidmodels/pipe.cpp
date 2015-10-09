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

#include "pipe.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

  
defineType(Pipe);
addToFactoryTable(SolidModel, Pipe, NoParameters);

Pipe::Pipe(const NoParameters& nop): SolidModel(nop)
{}


Pipe::Pipe(const SolidModel& spine, const SolidModel& xsec)
{
  if (!spine.isSingleWire())
    throw insight::Exception("spine feature has to provide a singly connected wire!");
  
  if (!xsec.isSingleFace() || xsec.isSingleWire() || xsec.isSingleEdge())
    throw insight::Exception("xsec feature has to provide a face or wire!");
  
  TopoDS_Wire spinew=spine.asSingleWire();
  TopoDS_Vertex pfirst, plast;
  TopExp::Vertices( spinew, pfirst, plast );
  
  
  gp_Trsf tr;
  tr.SetTranslation(gp_Vec(BRep_Tool::Pnt(pfirst).XYZ()));
  TopoDS_Shape xsecs=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr).Shape();

//   BRepOffsetAPI_MakePipeShell p(spinew);
//   Handle_Law_Constant law(new Law_Constant());
//   law->Set(1.0, -1e10, 1e10);
//   p.SetLaw(static_cast<TopoDS_Shape>(xsec), law, pfirst);
//   p.SetMode(true);
//   p.MakeSolid();
  
  BRepOffsetAPI_MakePipe p(spinew, xsecs);
  
  p.Build();
  setShape(p.Shape());
}

void Pipe::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Pipe",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression >> ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Pipe>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}

}
}
