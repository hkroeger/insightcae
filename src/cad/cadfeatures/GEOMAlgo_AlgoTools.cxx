// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  File    : GEOMAlgo_AlgoTools.cxx
//  Created :
//  Author  : Peter KURNEV

#include <GEOMAlgo_AlgoTools.hxx>

#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir2d.hxx>
#include <Bnd_Box.hxx>

#include <Geom2d_Curve.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>

#include <Geom2dHatch_Intersector.hxx>
#include <Geom2dHatch_Hatcher.hxx>

#include <Geom2dAdaptor_Curve.hxx>
#include <HatchGen_Domain.hxx>

#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>

#include <GeomAdaptor_Surface.hxx>

#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>

#include <Poly_Triangulation.hxx>

#include <TopAbs_Orientation.hxx>

#include <TopLoc_Location.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>

#include <TopExp_Explorer.hxx>

#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>

#include <BRepTools.hxx>
#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>

#include <IntTools_Tools.hxx>

#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>

#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopAbs_ShapeEnum.hxx>

#include <IntTools_Tools.hxx>

#include <BOPTools_AlgoTools3D.hxx>
#include <BOPTools_AlgoTools2D.hxx>

#include <GEOMAlgo_PassKeyShape.hxx>


static
  void GetCount(const TopoDS_Shape& aS,
                Standard_Integer& iCnt);
static
  void CopySource(const TopoDS_Shape& aS,
    TopTools_IndexedDataMapOfShapeShape& aMapSS,
    TopoDS_Shape& aSC);

//=======================================================================
//function : CopyShape
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::CopyShape(const TopoDS_Shape& aS,
       TopoDS_Shape& aSC)
{
  TopTools_IndexedDataMapOfShapeShape aMapSS;
  //
  CopySource(aS, aMapSS, aSC);
}
//=======================================================================
//function : CopyShape
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::CopyShape(const TopoDS_Shape& aS,
       TopoDS_Shape& aSC,
       TopTools_IndexedDataMapOfShapeShape& aMapSS)
{
  CopySource(aS, aMapSS, aSC);
}
//=======================================================================
//function : CopySource
//purpose  :
//=======================================================================
void CopySource(const TopoDS_Shape& aS,
                TopTools_IndexedDataMapOfShapeShape& aMapSS,
                TopoDS_Shape& aSC)
{
  Standard_Boolean bFree;
  TopAbs_ShapeEnum aT;
  TopoDS_Iterator aIt;
  TopoDS_Shape aSF;
  BRep_Builder BB;
  //
  aT=aS.ShapeType();
  //
  if (aMapSS.Contains(aS)) {
    aSC=aMapSS.ChangeFromKey(aS);
    aSC.Orientation(aS.Orientation());
    return;
  }
  else {
    aSC=aS.EmptyCopied();
    aMapSS.Add(aS, aSC);
  }
  //
  bFree=aSC.Free();
  aSC.Free(Standard_True);
  aSF=aS;
  if (aT==TopAbs_EDGE){
    TopAbs_Orientation aOr;
    //
    aOr=aS.Orientation();
    if(aOr==TopAbs_INTERNAL) {
      aSF.Orientation(TopAbs_FORWARD);
    }
  }
  aIt.Initialize(aSF);
  for (; aIt.More();  aIt.Next()) {
    TopoDS_Shape aSCx;
    //
    const TopoDS_Shape& aSx=aIt.Value();
    //
    CopySource (aSx, aMapSS, aSCx);
    //
    aSCx.Orientation(aSx.Orientation());
    BB.Add(aSC, aSCx);
  }
  aSC.Free(bFree);
}
//=======================================================================
//function : FaceNormal
//purpose  : 
//=======================================================================
void GEOMAlgo_AlgoTools::FaceNormal (const TopoDS_Face& aF,
         const Standard_Real U,
         const Standard_Real V,
         gp_Vec& aN)
{
  gp_Pnt aPnt ;
  gp_Vec aD1U, aD1V;
  Handle(Geom_Surface) aSurface;

  aSurface=BRep_Tool::Surface(aF);
  aSurface->D1 (U, V, aPnt, aD1U, aD1V);
  aN=aD1U.Crossed(aD1V);
  aN.Normalize();  
  if (aF.Orientation() == TopAbs_REVERSED){
    aN.Reverse();
  }
  return;
}
//=======================================================================
//function : BuildPCurveForEdgeOnFace
//purpose  :
//=======================================================================
Standard_Integer GEOMAlgo_AlgoTools::BuildPCurveForEdgeOnFace
  (const TopoDS_Edge& aEold,
   const TopoDS_Edge& aEnew,
   const TopoDS_Face& aF,
   const Handle(IntTools_Context)& aCtx)
{
  Standard_Boolean bIsClosed, bUClosed, bHasOld;
  Standard_Integer iRet, aNbPoints;
  Standard_Real aTS, aTS1, aTS2, aT, aT1, aT2, aScPr, aTol;
  Standard_Real aU, aV, aUS1, aVS1, aUS2, aVS2;
  gp_Pnt aP;
  gp_Pnt2d aP2DS1, aP2DS2, aP2D;
  gp_Vec2d aV2DS1, aV2DS2;
  Handle(Geom2d_Curve) aC2D, aC2DS1, aC2DS2;
  Handle(Geom_Surface) aS;
  TopoDS_Edge aES;
  //
  iRet=0;
  //
  bHasOld=BOPTools_AlgoTools2D::HasCurveOnSurface(aEnew, aF, aC2D, aT1, aT2, aTol);
  if (bHasOld) {
    return iRet;
  }
  //
  // Try to copy PCurve from old edge to the new one.
  iRet = BOPTools_AlgoTools2D::AttachExistingPCurve(aEold, aEnew, aF, aCtx);

  if (iRet) {
    // Do PCurve using projection algorithm.
    iRet = 0;
  } else {
    // The PCurve is attached successfully.
    return iRet;
  }
  //
  BOPTools_AlgoTools2D::BuildPCurveForEdgeOnFace(aEnew, aF);
  aC2D=BRep_Tool::CurveOnSurface(aEnew, aF, aT1, aT2);
  if (aC2D.IsNull()){
    iRet=1;
    return iRet;
  }
  //
  bIsClosed=BRep_Tool::IsClosed(aEold, aF);
  if (!bIsClosed) {
    return iRet;
  }
  //
  aTol=1.e-7;
  //
  // 1. bUClosed - direction of closeness
  //
  aES=aEold;
  aES.Orientation(TopAbs_FORWARD);
  aC2DS1=BRep_Tool::CurveOnSurface(aES, aF, aTS1, aTS2);
  //
  aES.Orientation(TopAbs_REVERSED);
  aC2DS2=BRep_Tool::CurveOnSurface(aES, aF, aTS1, aTS2);
  //
  aTS=IntTools_Tools::IntermediatePoint(aTS1, aTS2);
  //
  aC2DS1->D1(aTS, aP2DS1, aV2DS1);
  aC2DS2->D1(aTS, aP2DS2, aV2DS2);
  //
  gp_Vec2d aV2DS12(aP2DS1, aP2DS2);
  gp_Dir2d aD2DS12(aV2DS12);
  const gp_Dir2d& aD2DX=gp::DX2d();
  //
  aScPr=aD2DS12*aD2DX;
  bUClosed=Standard_True;
  if (fabs(aScPr) < aTol) {
    bUClosed=!bUClosed;
  }
  //
  // 2. aP2D - point on curve aC2D, that corresponds to aP2DS1
  aP2DS1.Coord(aUS1, aVS1);
  aP2DS2.Coord(aUS2, aVS2);
  //
  aS=BRep_Tool::Surface(aF);
  aS->D0(aUS1, aVS1, aP);
  //
  GeomAPI_ProjectPointOnCurve& aProjPC=aCtx->ProjPC(aEnew);
  //
  aProjPC.Perform(aP);
  aNbPoints=aProjPC.NbPoints();
  if (!aNbPoints) {
    iRet=2;
    return iRet;
  }
  //
  aT=aProjPC.LowerDistanceParameter();

  //
  // 3. Build the second 2D curve
  Standard_Boolean bRevOrder;
  gp_Vec2d aV2DT, aV2D;
  Handle(Geom2d_Curve) aC2Dnew;
  Handle(Geom2d_TrimmedCurve) aC2DTnew;
  BRep_Builder aBB;
  //
  aC2D->D1(aT, aP2D, aV2D);
  aP2D.Coord(aU, aV);
  //
  aC2Dnew=Handle(Geom2d_Curve)::DownCast(aC2D->Copy());
  aC2DTnew = new Geom2d_TrimmedCurve(aC2Dnew, aT1, aT2);
  //
  aV2DT=aV2DS12;
  if (!bUClosed) {    // V Closed
    if (fabs(aV-aVS2)<aTol) {
      aV2DT.Reverse();
    }
  }
  else {   // U Closed
    if (fabs(aU-aUS2)<aTol) {
      aV2DT.Reverse();
    }
  }
  //
  aC2DTnew->Translate(aV2DT);
  //
  // 4 Order the 2D curves
  bRevOrder=Standard_False;
  aScPr=aV2D*aV2DS1;
  if(aScPr<0.) {
    bRevOrder=!bRevOrder;
  }
  //
  // 5. Update the edge
  aTol=BRep_Tool::Tolerance(aEnew);
  if (!bRevOrder) {
    aBB.UpdateEdge(aEnew, aC2D, aC2DTnew, aF, aTol);
  }
  else {
    aBB.UpdateEdge(aEnew, aC2DTnew, aC2D , aF, aTol);
  }
  //
  return iRet;
}
//////////////////////////////////////////////////////////////////////////
//=======================================================================
// function: MakeContainer
// purpose:
//=======================================================================
void GEOMAlgo_AlgoTools::MakeContainer(const TopAbs_ShapeEnum theType,
           TopoDS_Shape& theC)
{
  BRep_Builder aBB;
  //
  switch(theType) {
    case TopAbs_COMPOUND:{
      TopoDS_Compound aC;
      aBB.MakeCompound(aC);
      theC=aC;
    }
      break;
      //
    case TopAbs_COMPSOLID:{
      TopoDS_CompSolid aCS;
      aBB.MakeCompSolid(aCS);
      theC=aCS;
    }
      break;
      //
    case TopAbs_SOLID:{
      TopoDS_Solid aSolid;
      aBB.MakeSolid(aSolid);
      theC=aSolid;
    }
      break;
      //
      //
    case TopAbs_SHELL:{
      TopoDS_Shell aShell;
      aBB.MakeShell(aShell);
      theC=aShell;
    }
      break;
      //
    case TopAbs_WIRE: {
      TopoDS_Wire aWire;
      aBB.MakeWire(aWire);
      theC=aWire;
    }
      break;
      //
    default:
      break;
  }
}
//=======================================================================
//function : IsUPeriodic
//purpose  :
//=======================================================================
Standard_Boolean GEOMAlgo_AlgoTools::IsUPeriodic(const  Handle(Geom_Surface) &aS)
{
  Standard_Boolean bRet;
  GeomAbs_SurfaceType aType;
  GeomAdaptor_Surface aGAS;
  //
  aGAS.Load(aS);
  aType=aGAS.GetType();
  bRet=(aType==GeomAbs_Cylinder||
        aType==GeomAbs_Cone ||
        aType==GeomAbs_Sphere);
  //
  return bRet;
}

//=======================================================================
//function : RefinePCurveForEdgeOnFace
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::RefinePCurveForEdgeOnFace(const TopoDS_Edge& aE,
         const TopoDS_Face& aF,
         const Standard_Real aUMin,
         const Standard_Real aUMax)
{
  Standard_Real aT1, aT2, aTx, aUx, aTol;
  gp_Pnt2d aP2D;
  Handle(Geom_Surface) aS;
  Handle(Geom2d_Curve) aC2D;
  BRep_Builder aBB;
  //
  aC2D=BRep_Tool::CurveOnSurface(aE, aF, aT1, aT2);
  if (!aC2D.IsNull()) {
    if (BRep_Tool::IsClosed(aE, aF)) {
      return;
    }
    aTx=IntTools_Tools::IntermediatePoint(aT1, aT2);
    aC2D->D0(aTx, aP2D);
    aUx=aP2D.X();
    if (aUx < aUMin || aUx > aUMax) {
      // need to rebuild
      Handle(Geom2d_Curve) aC2Dx;
      //
      aTol=BRep_Tool::Tolerance(aE);
      aBB.UpdateEdge(aE, aC2Dx, aF, aTol);
    }
  }
}
//=======================================================================
//function :IsSplitToReverse
//purpose  : 
//=======================================================================
Standard_Boolean GEOMAlgo_AlgoTools::IsSplitToReverse
  (const TopoDS_Edge& aEF1,
   const TopoDS_Edge& aEF2,
   const Handle(IntTools_Context)& aContext)
{
  Standard_Boolean aFlag;
  Standard_Real aT1, aT2, aScPr, a, b;
  gp_Vec aV1, aV2;
  gp_Pnt aP;
  
  
  Handle(Geom_Curve)aC1=BRep_Tool::Curve(aEF1, a, b);
  aT1=IntTools_Tools::IntermediatePoint(a, b);
  aC1->D0(aT1, aP);
  aFlag=BOPTools_AlgoTools2D::EdgeTangent(aEF1, aT1, aV1);

  if(!aFlag) {
    return Standard_False;
  }

  gp_Dir aDT1(aV1);
  //
  aFlag=aContext->ProjectPointOnEdge(aP, aEF2, aT2);
  if(!aFlag) {
    return Standard_False;
  }
  //
  aFlag=BOPTools_AlgoTools2D::EdgeTangent(aEF2, aT2, aV2);
  if(!aFlag) {
    return Standard_False;
  }

  gp_Dir aDT2(aV2);

  aScPr=aDT1*aDT2;

  return (aScPr<0.);
}


//=======================================================================
//function : ProjectPointOnShape
//purpose  :
//=======================================================================
Standard_Boolean GEOMAlgo_AlgoTools::ProjectPointOnShape
  (const gp_Pnt& aP1,
   const TopoDS_Shape& aS,
   gp_Pnt& aP2,
   const Handle(IntTools_Context)& aCtx)
{
  Standard_Boolean bIsDone = Standard_False;
  Standard_Real aT2;
  TopAbs_ShapeEnum aType;
  //
  aType = aS.ShapeType();
  switch (aType)
    {
    case TopAbs_EDGE:
      {
        const TopoDS_Edge& aE2 = TopoDS::Edge(aS);
        //
        if (BRep_Tool::Degenerated(aE2)) { // jfa
          return Standard_True;
        }
        else {
          Standard_Real f, l;
          Handle(Geom_Curve) aC3D = BRep_Tool::Curve (aE2, f, l);
          if (aC3D.IsNull()) {
            return Standard_True;
          }
          bIsDone = aCtx->ProjectPointOnEdge(aP1, aE2, aT2);
        }
        if (!bIsDone) {
          return bIsDone;
        }
        //
        GEOMAlgo_AlgoTools::PointOnEdge(aE2, aT2, aP2);
      }
      break;
      //
    case TopAbs_FACE:
      {
        const TopoDS_Face& aF2 = TopoDS::Face(aS);
        GeomAPI_ProjectPointOnSurf& aProj = aCtx->ProjPS(aF2);
        //
        aProj.Perform(aP1);
        bIsDone = aProj.IsDone();
        if (!bIsDone) {
          return bIsDone;
        }
        //
        aP2 = aProj.NearestPoint();
      }
      break;
      //
    default:
      break; // Err
    }
  return bIsDone;
}

//=======================================================================
//function : PointOnEdge
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::PointOnEdge(const TopoDS_Edge& aE,
         gp_Pnt& aP3D)
{
  Standard_Real aTx, aT1, aT2;
  //
  BRep_Tool::Curve(aE, aT1, aT2);
  aTx=IntTools_Tools::IntermediatePoint(aT1, aT2);
  GEOMAlgo_AlgoTools::PointOnEdge(aE, aTx, aP3D);
}
//=======================================================================
//function : PointOnEdge
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::PointOnEdge(const TopoDS_Edge& aE,
         const Standard_Real aT,
         gp_Pnt& aP3D)
{
  Standard_Real aT1, aT2;
  Handle(Geom_Curve) aC3D;
  //
  aC3D=BRep_Tool::Curve(aE, aT1, aT2);
  aC3D->D0(aT, aP3D);
}
//=======================================================================
//function : PointOnFace
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::PointOnFace(const TopoDS_Face& aF,
         const Standard_Real aU,
         const Standard_Real aV,
         gp_Pnt& aP3D)
{
  Handle(Geom_Surface) aS;
  //
  aS=BRep_Tool::Surface(aF);
  aS->D0(aU, aV, aP3D);
}
//=======================================================================
//function : PointOnFace
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::PointOnFace(const TopoDS_Face& aF,
         gp_Pnt& aP3D)
{
  Standard_Real aU, aV, aUMin, aUMax, aVMin, aVMax;
  //
  BRepTools::UVBounds(aF, aUMin, aUMax, aVMin, aVMax);
  //
  aU=IntTools_Tools::IntermediatePoint(aUMin, aUMax);
  aV=IntTools_Tools::IntermediatePoint(aVMin, aVMax);
  //
  GEOMAlgo_AlgoTools::PointOnFace(aF, aU, aV, aP3D);
}
//=======================================================================
//function : PointOnShape
//purpose  :
//=======================================================================
void GEOMAlgo_AlgoTools::PointOnShape(const TopoDS_Shape& aS,
          gp_Pnt& aP3D)
{
  TopAbs_ShapeEnum aType;
  //
  aP3D.SetCoord(99.,99.,99.);
  aType=aS.ShapeType();
  switch(aType) {
    case TopAbs_EDGE: {
      const TopoDS_Edge& aE=TopoDS::Edge(aS);
      GEOMAlgo_AlgoTools::PointOnEdge(aE, aP3D);
      }
      break;
      //
    case TopAbs_FACE: {
      const TopoDS_Face& aF=TopoDS::Face(aS);
      GEOMAlgo_AlgoTools::PointOnFace(aF, aP3D);
      }
      break;
      //
    default:
      break; // Err
  }
}
//=======================================================================
//function : FindSDShapes
//purpose  :
//=======================================================================
Standard_Integer GEOMAlgo_AlgoTools::FindSDShapes
  (const TopoDS_Shape& aE1,
   const TopTools_ListOfShape& aLE,
   const Standard_Real aTol,
   TopTools_ListOfShape& aLESD,
   const Handle(IntTools_Context)& aCtx)
{
  Standard_Boolean bIsDone;
  Standard_Real aTol2, aD2;
  gp_Pnt aP1, aP2;
  TopTools_ListIteratorOfListOfShape aIt;
  //
  aTol2=aTol*aTol;
  GEOMAlgo_AlgoTools::PointOnShape(aE1, aP1);
  //
  aIt.Initialize(aLE);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aE2=aIt.Value();
    if (aE2.IsSame(aE1)) {
       aLESD.Append(aE2);
    }
    else {
      bIsDone=GEOMAlgo_AlgoTools::ProjectPointOnShape(aP1, aE2, aP2, aCtx);
      if (!bIsDone) {
        //return 1;
        continue; // jfa BUG 20361
      }
      aD2=aP1.SquareDistance(aP2);
      if(aD2<aTol2) {
        aLESD.Append(aE2);
      }
    }
  }
  return 0;
}

//=======================================================================
//function : FindSDShapes
//purpose  :
//=======================================================================
Standard_Integer GEOMAlgo_AlgoTools::FindSDShapes
  (const TopTools_ListOfShape& aLE,
   const Standard_Real aTol,
   TopTools_IndexedDataMapOfShapeListOfShape& aMEE,
   const Handle(IntTools_Context)& aCtx)
{
  Standard_Integer aNbE, aNbEProcessed, aNbESD, iErr;
  TopTools_ListOfShape aLESD;
  TopTools_ListIteratorOfListOfShape aIt, aIt1;
  TopTools_IndexedMapOfShape aMProcessed;
  TopAbs_ShapeEnum aType;
  //
  aNbE=aLE.Extent();
  if (!aNbE) {
    return 3; // Err
  }
  if (aNbE==1) {
    return 0; // Nothing to do
  }
  //
  for(;;) {
    aNbEProcessed=aMProcessed.Extent();
    if (aNbEProcessed==aNbE) {
      break;
    }
    //
    aIt.Initialize(aLE);
    for (; aIt.More(); aIt.Next()) {
      const TopoDS_Shape& aS=aIt.Value();
      //
      if (aMProcessed.Contains(aS)) {
        continue;
      }
      //
      aType=aS.ShapeType();
      if (aType==TopAbs_EDGE) {
        const TopoDS_Edge& aE=TopoDS::Edge(aS);
        if (BRep_Tool::Degenerated(aE)) {
          aMProcessed.Add(aE);
          continue;
        }
      }
      //
      aLESD.Clear();
      iErr=GEOMAlgo_AlgoTools::FindSDShapes(aS, aLE, aTol, aLESD, aCtx);
      if (iErr) {
        return 2; // Err
      }
      //
      aNbESD=aLESD.Extent();
      if (!aNbESD) {
        return 1; // Err
      }
      //
      aMEE.Add(aS, aLESD);
      //
      aIt1.Initialize(aLESD);
      for (; aIt1.More(); aIt1.Next()) {
        const TopoDS_Shape& aE1=aIt1.Value();
        aMProcessed.Add(aE1);
      }
    }
  }
  return 0;
}
//=======================================================================
//function : RefineSDShapes
//purpose  :
//=======================================================================
Standard_Integer GEOMAlgo_AlgoTools::RefineSDShapes
  (GEOMAlgo_IndexedDataMapOfPassKeyShapeListOfShape& aMPKLE,
   const Standard_Real aTol,
   const Handle(IntTools_Context)& aCtx)
{
  Standard_Integer i, aNbE, iErr, j, aNbEE, aNbToAdd;
  TopTools_IndexedDataMapOfShapeListOfShape aMEE, aMSDE, aMEToAdd;
  //
  iErr=1;
  //
  aNbE=aMPKLE.Extent();
  for (i=1; i<=aNbE; ++i) {
    TopTools_ListOfShape& aLSDE=aMPKLE.ChangeFromIndex(i);
    //
    aMEE.Clear();
    iErr=GEOMAlgo_AlgoTools::FindSDShapes(aLSDE, aTol, aMEE, aCtx);
    if (iErr) {
      return iErr;
    }
    //
    aNbEE=aMEE.Extent();
    if (aNbEE==1) {
      continue;  // nothing to do
    }
    //
    for (j=1; j<=aNbEE; ++j) {
      TopTools_ListOfShape& aLEE=aMEE.ChangeFromIndex(j);
      //
      if (j==1) {
        aLSDE.Clear();
        aLSDE.Append(aLEE);
      }
      else {
        const TopoDS_Shape& aE1=aLEE.First();
        aMEToAdd.Add(aE1, aLEE);
      }
    }
  }
  //
  aNbToAdd=aMEToAdd.Extent();
  if (!aNbToAdd) {
    return aNbToAdd;
  }
  //
  for (i=1; i<=aNbToAdd; ++i) {
    GEOMAlgo_PassKeyShape aPKE1;
    //
    const TopoDS_Shape& aE1=aMEToAdd.FindKey(i);
    const TopTools_ListOfShape& aLE=aMEToAdd(i);
    //
    aPKE1.SetShapes(aE1);
    aMPKLE.Add(aPKE1, aLE);
  }
  //
  return 0;
}
//=======================================================================
//function : BuildTriangulation
//purpose  :
//=======================================================================
Standard_Boolean 
  GEOMAlgo_AlgoTools::BuildTriangulation (const TopoDS_Shape& theShape)
{
  // calculate deflection
  Standard_Real aDeviationCoefficient = 0.001;

  Bnd_Box B;
  BRepBndLib::Add(theShape, B);
  Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
  B.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);

  Standard_Real dx = aXmax - aXmin, dy = aYmax - aYmin, dz = aZmax - aZmin;
  Standard_Real aDeflection = Max(Max(dx, dy), dz) * aDeviationCoefficient * 4;
  Standard_Real aHLRAngle = 0.349066;

  // build triangulation
  BRepMesh_IncrementalMesh Inc (theShape, aDeflection, Standard_False, aHLRAngle);

  // check triangulation
  bool isTriangulation = true;

  TopExp_Explorer exp (theShape, TopAbs_FACE);
  if (exp.More())
  {
    TopLoc_Location aTopLoc;
    Handle(Poly_Triangulation) aTRF;
    aTRF = BRep_Tool::Triangulation(TopoDS::Face(exp.Current()), aTopLoc);
    if (aTRF.IsNull()) {
      isTriangulation = false;
    }
  }
  else // no faces, try edges
  {
    TopExp_Explorer expe (theShape, TopAbs_EDGE);
    if (!expe.More()) {
      isTriangulation = false;
    }
    else {
      TopLoc_Location aLoc;
      Handle(Poly_Polygon3D) aPE = BRep_Tool::Polygon3D(TopoDS::Edge(expe.Current()), aLoc);
      if (aPE.IsNull()) {
        isTriangulation = false;
      }
    }
  }
  return isTriangulation;
}

//=======================================================================
//function : IsCompositeShape
//purpose  :
//=======================================================================
Standard_Boolean GEOMAlgo_AlgoTools::IsCompositeShape(const TopoDS_Shape& aS)
{
  Standard_Boolean bRet;
  Standard_Integer iCnt;
  TopoDS_Iterator aIt;
  //
  iCnt=0;
  GetCount(aS, iCnt);
  bRet=(iCnt>1);
  //
  return bRet;
}
//=======================================================================
//function : GetCount
//purpose  :
//=======================================================================
void GetCount(const TopoDS_Shape& aS,
              Standard_Integer& iCnt)
{
  TopoDS_Iterator aIt;
  TopAbs_ShapeEnum aTS;
  //
  aTS=aS.ShapeType();
  //
  if (aTS==TopAbs_SHAPE) {
    return;
  }
  if (aTS!=TopAbs_COMPOUND) {
    ++iCnt;
    return;
  }
  //
  aIt.Initialize(aS);
  for (; aIt.More(); aIt.Next()) {
    const TopoDS_Shape& aSx=aIt.Value();
    GetCount(aSx, iCnt);
  }
}
//=======================================================================
//function : PntInFace
//purpose  :
//=======================================================================
Standard_Integer GEOMAlgo_AlgoTools::PntInFace(const TopoDS_Face& aF,
                                               gp_Pnt& theP,
                                               gp_Pnt2d& theP2D)
{
  Standard_Boolean bIsDone, bHasFirstPoint, bHasSecondPoint;
  Standard_Integer iErr, aIx, aNbDomains, i;
  Standard_Real aUMin, aUMax, aVMin, aVMax;
  Standard_Real aVx, aUx, aV1, aV2, aU1, aU2, aEpsT;
  Standard_Real aTotArcIntr, aTolTangfIntr, aTolHatch2D, aTolHatch3D;
  gp_Dir2d aD2D (0., 1.);
  gp_Pnt2d aP2D;
  gp_Pnt aPx;
  Handle(Geom2d_Curve) aC2D;
  Handle(Geom2d_TrimmedCurve) aCT2D;
  Handle(Geom2d_Line) aL2D;
  Handle(Geom_Surface) aS;
  TopAbs_Orientation aOrE;
  TopoDS_Face aFF;
  TopExp_Explorer aExp;
  //
  aTolHatch2D=1.e-8;
  aTolHatch3D=1.e-8;
  aTotArcIntr=1.e-10;
  aTolTangfIntr=1.e-10;
  //
  Geom2dHatch_Intersector aIntr(aTotArcIntr, aTolTangfIntr);
  Geom2dHatch_Hatcher aHatcher(aIntr,
          aTolHatch2D, aTolHatch3D,
          Standard_True, Standard_False);
  //
  iErr=0;
  aEpsT=1.e-12;
  //
  aFF=aF;
  aFF.Orientation (TopAbs_FORWARD);
  //
  aS=BRep_Tool::Surface(aFF);
  BRepTools::UVBounds(aFF, aUMin, aUMax, aVMin, aVMax);
  //
  // 1
  aExp.Init (aFF, TopAbs_EDGE);
  for (; aExp.More() ; aExp.Next()) {
    const TopoDS_Edge& aE=*((TopoDS_Edge*)&aExp.Current());
    aOrE=aE.Orientation();
    //
    aC2D=BRep_Tool::CurveOnSurface (aE, aFF, aU1, aU2);
    if (aC2D.IsNull() ) {
      iErr=1;
      return iErr;
    }
    if (fabs(aU1-aU2) < aEpsT) {
      iErr=2;
      return iErr;
    }
    //
    aCT2D=new Geom2d_TrimmedCurve(aC2D, aU1, aU2);
    aHatcher.AddElement(aCT2D, aOrE);
  }// for (; aExp.More() ; aExp.Next()) {
  //
  // 2
  aUx=IntTools_Tools::IntermediatePoint(aUMin, aUMax);
  aP2D.SetCoord(aUx, 0.);
  aL2D=new Geom2d_Line (aP2D, aD2D);
  Geom2dAdaptor_Curve aHCur(aL2D);
  //
  aIx=aHatcher.AddHatching(aHCur) ;
  //
  // 3.
  aHatcher.Trim();
  bIsDone=aHatcher.TrimDone(aIx);
  if (!bIsDone) {
    iErr=3;
    return iErr;
  }
  //
  aHatcher.ComputeDomains(aIx);
  bIsDone=aHatcher.IsDone(aIx);
  if (!bIsDone) {
    iErr=4;
    return iErr;
  }
  //
  // 4.
  aVx=aVMin;
  aNbDomains=aHatcher.NbDomains(aIx);
  if (!aNbDomains) {
    iErr=5;
    return iErr;
  }
  //
  i=1;
  const HatchGen_Domain& aDomain=aHatcher.Domain (aIx, i) ;
  bHasFirstPoint=aDomain.HasFirstPoint();
  if (!bHasFirstPoint) {
    iErr=5;
    return iErr;
  }
  //
  aV1=aDomain.FirstPoint().Parameter();
  //
  bHasSecondPoint=aDomain.HasSecondPoint();
  if (!bHasSecondPoint) {
    iErr=6;
    return iErr;
  }
  //
  aV2=aDomain.SecondPoint().Parameter();
  //
  aVx=IntTools_Tools::IntermediatePoint(aV1, aV2);
  //
  aS->D0(aUx, aVx, aPx);
  //
  theP2D.SetCoord(aUx, aVx);
  theP=aPx;
  //
  return iErr;
}
