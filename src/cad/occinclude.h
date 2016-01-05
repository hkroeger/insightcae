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
 *
 */

#ifndef OCC_INCLUDE
#define OCC_INCLUDE

#undef Status

#include "Standard_Version.hxx"

#include <Quantity_Color.hxx>
#include <Quantity_TypeOfColor.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#include <BRep_Tool.hxx>
#include "BRepExtrema_DistShapeShape.hxx"
#include "BRepExtrema_ExtPF.hxx"
#include <Geom_Curve.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <BRepTools.hxx>
#include <TopExp.hxx>
#include <BRepProj_Projection.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeShell.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepOffsetAPI_Sewing.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <Poly_Triangle.hxx>
#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>
#include <BRepGProp.hxx>
#include <BRepGProp_Face.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <TopExp.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_XYZ.hxx>
#include <gp_Vec.hxx>
#include <gp_Elips.hxx>
#include <gp_Circ.hxx>
#include "gp_Cylinder.hxx"
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <Geom_Curve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepTools.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_DataMapOfShapeInteger.hxx>
#include <TopExp.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeShell.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_Sewing.hxx>
#include "BRepOffsetAPI_MakeOffsetShape.hxx"
#include <BRepLProp_CLProps.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangle.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <IGESControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <IGESToBRep_Reader.hxx>
#include <Interface_Static.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>
#include <ShapeUpgrade_ShellSewing.hxx>
#include <ShapeFix_Edge.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeFix_Wireframe.hxx>
#include "ShapeAnalysis_FreeBounds.hxx"
#include <BRepMesh.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <ShapeAnalysis.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <IGESControl_Writer.hxx>
#include <STEPControl_Writer.hxx>
#include <StlAPI_Writer.hxx>
#include <STEPControl_StepModelType.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <ShapeAnalysis_ShapeContents.hxx>
#include <ShapeAnalysis_CheckSmallFace.hxx>
#include <ShapeAnalysis_DataMapOfShapeListOfReal.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepLib.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <ShapeFix.hxx>
#include <ShapeFix_FixSmallFace.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <Precision.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepFill.hxx>
// #include "BRepFill_Pipe.hxx"
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <IGESControl_Writer.hxx>
#include <IGESControl_Controller.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include "BRepOffsetAPI_MakePipe.hxx"
#include "BRepOffsetAPI_MakePipeShell.hxx"
#include "Law_Constant.hxx"
#include "Law_Linear.hxx"

#include "GC_MakePlane.hxx"
#include "GC_MakeCircle.hxx"
#include "GC_MakeArcOfCircle.hxx"
#include "GC_MakeSegment.hxx"
#include "BRepMesh_FastDiscret.hxx"
#include "ShapeAnalysis_Curve.hxx"
#include "ShapeAnalysis_Surface.hxx"
#include "GeomLProp_SLProps.hxx"
#include "GCPnts_UniformDeflection.hxx"
#include "GCPnts_QuasiUniformDeflection.hxx"
#include "GCPnts_UniformAbscissa.hxx"
#include "GCPnts_QuasiUniformAbscissa.hxx"
#include "gce_MakeCirc.hxx"
#include "GCE2d_MakeSegment.hxx"

#include "HLRBRep_Algo.hxx"
#include "HLRBRep_HLRToShape.hxx"

#include "ElCLib.hxx"
#include "Geom_Circle.hxx"
#include "Geom_TrimmedCurve.hxx"
#include "Geom_CartesianPoint.hxx"

#include "BRepAdaptor_CompCurve.hxx"

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_ConnectedInteractive.hxx>
#include <AIS_MultipleConnectedInteractive.hxx>
#include <AIS_LengthDimension.hxx>
#include <AIS_RadiusDimension.hxx>
#include <AIS_Shape.hxx>

#include "GEOMAlgo_Splitter.hxx"

#include <armadillo>


inline gp_Pnt to_Pnt(const arma::mat& xyz)
{
  return gp_Pnt(xyz(0), xyz(1), xyz(2));
}

inline gp_Vec to_Vec(const arma::mat& xyz)
{
  return gp_Vec(xyz(0), xyz(1), xyz(2));
}

#endif