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

#include "projectedoutline.h"
#include "datum.h"
#include "BRepOffsetAPI_NormalProjection.hxx"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {



defineType(ProjectedOutline);
addToFactoryTable(SolidModel, ProjectedOutline, NoParameters);

ProjectedOutline::ProjectedOutline(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeOutlineProjection
(
  const SolidModel& source, 
  const Datum& target
)
{
  if (!target.providesPlanarReference())
    throw insight::Exception("Error: Wrong parameter. ProjectedOutline needs a planar reference!");
  
  gp_Ax3 pln=target;
  
  HLRAlgo_Projector projector( pln.Ax2() );
  gp_Trsf transform=projector.FullTransformation();
  
  
  Handle_HLRBRep_Algo brep_hlr = new HLRBRep_Algo;
  brep_hlr->Add( source );
  brep_hlr->Projector( projector );
  brep_hlr->Update();
  brep_hlr->Hide();

  // extracting the result sets:
  HLRBRep_HLRToShape shapes( brep_hlr );
  
  TopoDS_Compound allVisible;
  BRep_Builder builder;
  builder.MakeCompound( allVisible );
  TopoDS_Shape vs=shapes.VCompound();
  if (!vs.IsNull()) builder.Add(allVisible, vs);
//   TopoDS_Shape r1vs=shapes.Rg1LineVCompound();
//   if (!r1vs.IsNull()) builder.Add(allVisible, r1vs);
  TopoDS_Shape olvs = shapes.OutLineVCompound();
  if (!olvs.IsNull()) builder.Add(allVisible, olvs);
  
  return allVisible;
}


TopoDS_Shape makeOutlineProjectionEdges
(
  const SolidModel& source, 
  const Datum& target
)
{
  if (!target.providesPlanarReference())
    throw insight::Exception("Error: Wrong parameter. ProjectedOutline needs a planar reference!");
  
  gp_Ax3 pln=target;
  cout<<"pl"<<endl;
  TopoDS_Face face=BRepBuilderAPI_MakeFace(gp_Pln(pln));
  gp_Trsf trsf;
  trsf.SetTransformation(
    pln,
    gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0))
  );
  
  TopoDS_Compound allVisible;
  BRep_Builder builder;
  builder.MakeCompound( allVisible );

  TopoDS_Shape src=source;
  for (TopExp_Explorer ex(src, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge edge=TopoDS::Edge(ex.Current());
    BRepLib::BuildCurve3d(edge); // just to be sure...

    //     BRepProj_Projection proj(edge, face, pln.Direction().Reversed());
    BRepOffsetAPI_NormalProjection proj(face);
    proj.SetLimit(false);
    proj.Add(edge);
    proj.Build();

    BRepBuilderAPI_Transform tr( proj.Shape(), trsf.Inverted() );

    builder.Add(allVisible, tr.Shape());
  }

  return allVisible;
}

ProjectedOutline::ProjectedOutline(const SolidModel& source, const Datum& target)
//: SolidModel(makeOutlineProjection(source, target))
{
  if (source.allFaces().size()==0)
    setShape(makeOutlineProjectionEdges(source, target));
  else
    setShape(makeOutlineProjection(source, target));
}

void ProjectedOutline::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "ProjectedOutline",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_datumExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<ProjectedOutline>(*qi::_1, *qi::_2)) ]
      
    ))
  );
}

}
}
