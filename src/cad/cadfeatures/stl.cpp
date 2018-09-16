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

#include "stl.h"

#include "StlAPI.hxx"

#include <StlAPI_Reader.hxx>

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <gp_Pnt.hxx>
#include <OSD_Path.hxx>
#include <RWStl.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <MeshVS_Drawer.hxx>
#include <MeshVS_MeshPrsBuilder.hxx>
#include <MeshVS_DrawerAttribute.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <XSDRAWSTLVRML_DataSource.hxx>
#include <AIS_ColoredShape.hxx>
#include <TColStd_HPackedMapOfInteger.hxx>

#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {




defineType(STL);
addToFactoryTable(Feature, STL);


size_t STL::calcHash() const
{
  ParameterListHash h;
  h+=fname_;
  return h.getHash();
}


STL::STL()
{}




STL::STL
(
  const boost::filesystem::path& fname
)
: fname_(fname)
{
}



FeaturePtr STL::create
(
    const boost::filesystem::path& fname
)
{
    return FeaturePtr(new STL(fname));
}



Handle_AIS_InteractiveObject STL::buildVisualization() const
{
    checkForBuildDuringAccess();
    return Handle_AIS_InteractiveObject::DownCast(mesh_);
}

void STL::build()
{
  ExecTimer t("STL::build() ["+featureSymbolName()+"]");


  Handle(Poly_Triangulation) aSTLMesh = RWStl::ReadFile (fname_.c_str());
  if (aSTLMesh.IsNull())
  {
    return Standard_False;
  }

  mesh_= new MeshVS_Mesh;
  Handle(MeshVS_DataSource) M( new XSDRAWSTLVRML_DataSource ( aSTLMesh) );
  mesh_->SetDataSource(M);

  Handle_MeshVS_MeshPrsBuilder Prs( new MeshVS_MeshPrsBuilder(mesh_) );
  mesh_->AddBuilder(Prs, Standard_True );//False -> No selection

  mesh_->GetDrawer()->SetColor( MeshVS_DA_EdgeColor, Quantity_NOC_BLACK );

//  Handle(TColStd_HPackedMapOfInteger) aNodes = new TColStd_HPackedMapOfInteger();
//  Standard_Integer aLen = aSTLMesh->Nodes().Length();
//  for ( Standard_Integer anIndex = 1; anIndex <= aLen; anIndex++ )
//    aNodes->ChangeMap().Add( anIndex );
//  mesh_->SetHiddenNodes( aNodes );
//  mesh_->SetSelectableNodes ( aNodes );

  mesh_->GetDrawer()->SetBoolean(MeshVS_DA_DisplayNodes, Standard_False); //MeshVS_DrawerAttribute
  mesh_->GetDrawer()->SetBoolean(MeshVS_DA_ShowEdges, Standard_True);
  mesh_->GetDrawer()->SetBoolean ( MeshVS_DA_Reflection, Standard_True );
  mesh_->GetDrawer()->SetBoolean ( MeshVS_DA_SmoothShading, Standard_True);
  mesh_->GetDrawer()->SetMaterial(MeshVS_DA_FrontMaterial, Graphic3d_NOM_BRASS);

  mesh_->SetColor(Quantity_NOC_BLACK);
  mesh_->SetDisplayMode( MeshVS_DMF_Shading ); // Mode as defaut
  mesh_->SetHilightMode( MeshVS_DMF_WireFrame ); // Wireframe as default hilight mode

  setShape( TopoDS_Compound() );
}




void STL::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "STL",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '('
        >> ruleset.r_path >> ')' )
      [ qi::_val = phx::bind(&STL::create, qi::_1) ]

    ))
  );
}



FeatureCmdInfoList STL::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "STL",

            "( <path:filename> )",

            "Import a triangulated surface for display."
        )
    );
}



}
}
