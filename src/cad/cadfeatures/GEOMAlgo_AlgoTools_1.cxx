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
//  File    : GEOMAlgo_AlgoTools_2.cxx
//  Created :
//  Author  : Peter KURNEV

#include <GEOMAlgo_AlgoTools.hxx>

#include <GEOMAlgo_ListOfCoupleOfShapes.hxx>
#include <GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <GEOMAlgo_CoupleOfShapes.hxx>
#include <TopoDS_Shape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>



static
  void ProcessBlock(const TopoDS_Shape& aF,
                    const GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape& aMCV,
                    TopTools_IndexedMapOfShape& aProcessed,
                    TopTools_IndexedMapOfShape& aChain);

//=======================================================================
// function: FindChains
// purpose :
//=======================================================================
void GEOMAlgo_AlgoTools::FindChains(const GEOMAlgo_ListOfCoupleOfShapes& aLCS,
				    GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape& aMapChains)
{
  GEOMAlgo_ListIteratorOfListOfCoupleOfShapes aItCS;
  GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape aMCV;
  //
  aItCS.Initialize(aLCS);
  for (; aItCS.More(); aItCS.Next()) {
    const GEOMAlgo_CoupleOfShapes& aCS=aItCS.Value();
    //
    const TopoDS_Shape& aF1=aCS.Shape1();
    const TopoDS_Shape& aF2=aCS.Shape2();
    //
    //
    if (aMCV.Contains(aF1)) {
      TopTools_IndexedMapOfShape& aMV=aMCV.ChangeFromKey(aF1);
      aMV.Add(aF1);
      aMV.Add(aF2);
    }
    else {
      TopTools_IndexedMapOfShape aMV;
      aMV.Add(aF1);
      aMV.Add(aF2);
      aMCV.Add(aF1, aMV);
    }
    //
    if (aMCV.Contains(aF2)) {
      TopTools_IndexedMapOfShape& aMV=aMCV.ChangeFromKey(aF2);
      aMV.Add(aF1);
      aMV.Add(aF2);
    }
    else {
      TopTools_IndexedMapOfShape aMV;
      aMV.Add(aF1);
      aMV.Add(aF2);
      aMCV.Add(aF2, aMV);
    }
  }
  GEOMAlgo_AlgoTools::FindChains(aMCV, aMapChains);
}
//=======================================================================
// function: FindChains
// purpose :
//=======================================================================
void GEOMAlgo_AlgoTools::FindChains(const GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape& aMCV,
				    GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape& aMapChains)
{
  Standard_Integer  i, j, aNbCV, aNbV;
  TopTools_IndexedMapOfShape aProcessed, aChain;
  //
  aNbCV=aMCV.Extent();
  for (i=1; i<=aNbCV; ++i) {
    const TopoDS_Shape& aF=aMCV.FindKey(i);
    if (aProcessed.Contains(aF)) {
      continue;
    }
    //
    aProcessed.Add(aF);
    aChain.Add(aF);
    //
    const TopTools_IndexedMapOfShape& aMV=aMCV(i);
    aNbV=aMV.Extent();
    for (j=1; j<=aNbV; ++j) {
      const TopoDS_Shape& aFx=aMV(j);
      ProcessBlock(aFx, aMCV, aProcessed, aChain);
    }
    aMapChains.Add(aF, aChain);
    aChain.Clear();
  }
}
//=======================================================================
// function: ProcessBlock
// purpose:
//=======================================================================
void ProcessBlock(const TopoDS_Shape& aF,
                  const GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape& aMCV,
                  TopTools_IndexedMapOfShape& aProcessed,
                  TopTools_IndexedMapOfShape& aChain)
{
  Standard_Integer j, aNbV;
  //
  if (aProcessed.Contains(aF)) {
    return;
  }
  aProcessed.Add(aF);
  aChain.Add(aF);
  //
  const TopTools_IndexedMapOfShape& aMV=aMCV.FindFromKey(aF);
  aNbV=aMV.Extent();
  for (j=1; j<=aNbV; ++j) {
    const TopoDS_Shape& aFx=aMV(j);
    ProcessBlock(aFx, aMCV, aProcessed, aChain);
  }
}

