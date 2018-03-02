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




void STL::build()
{
  ExecTimer t("STL::build() ["+featureSymbolName()+"]");


  Handle(Poly_Triangulation) aMesh = RWStl::ReadFile (fname_.c_str());
  if (aMesh.IsNull())
  {
    return Standard_False;
  }

  TopoDS_Vertex aTriVertexes[3];
  TopoDS_Face aFace;
  TopoDS_Wire aWire;
  BRepBuilderAPI_Sewing aSewingTool;
  aSewingTool.Init (1.0e-06, Standard_True);

  TopoDS_Compound aComp;
  BRep_Builder BuildTool;
  BuildTool.MakeCompound (aComp);

  const TColgp_Array1OfPnt& aNodes = aMesh->Nodes();
  const Poly_Array1OfTriangle& aTriangles = aMesh->Triangles();
  for (Standard_Integer aTriIdx  = aTriangles.Lower();
                        aTriIdx <= aTriangles.Upper();
                      ++aTriIdx)
  {
    const Poly_Triangle& aTriangle = aTriangles(aTriIdx);

    Standard_Integer anId[3];
    aTriangle.Get(anId[0], anId[1], anId[2]);

    const gp_Pnt& aPnt1 = aNodes (anId[0]);
    const gp_Pnt& aPnt2 = aNodes (anId[1]);
    const gp_Pnt& aPnt3 = aNodes (anId[2]);
    if ((!(aPnt1.IsEqual (aPnt2, 0.0)))
     && (!(aPnt1.IsEqual (aPnt3, 0.0))))
    {
      aTriVertexes[0] = BRepBuilderAPI_MakeVertex (aPnt1);
      aTriVertexes[1] = BRepBuilderAPI_MakeVertex (aPnt2);
      aTriVertexes[2] = BRepBuilderAPI_MakeVertex (aPnt3);

      aWire = BRepBuilderAPI_MakePolygon (aTriVertexes[0], aTriVertexes[1], aTriVertexes[2], Standard_True);
      if (!aWire.IsNull())
      {
        aFace = BRepBuilderAPI_MakeFace (aWire);
        if (!aFace.IsNull())
        {
          BuildTool.Add (aComp, aFace);
        }
      }
    }
  }


//  aSewingTool.Load( aComp );
//  aSewingTool.Perform();
//  aShape = aSewingTool.SewedShape();
//  if ( aShape.IsNull() )
//    aShape = aComp;


/* v1
  TopoDS_Shape aShape;

  StlAPI::Read(aShape, fname_.c_str());
  */

  setShape(aComp);
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
