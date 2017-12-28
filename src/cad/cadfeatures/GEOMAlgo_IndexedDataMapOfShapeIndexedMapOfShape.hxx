// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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
// File:	GEOMAlgo_IndexedDataMapOfIndexedMapOfInteger.hxx
// Created:	Mon Feb 20 09:20:07 2012
// Author:
//		<pkv@BDEURI37616>
// File:	GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape.hxx
// Created:	Mon Feb 20 11:59:23 2012
// Author:
//		<pkv@BDEURI37616>


#ifndef GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape_HeaderFile
#define GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape_HeaderFile


#include <TopoDS_Shape.hxx>
#include <TopTools_ShapeMapHasher.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#define _NCollection_MapHasher
#include <NCollection_IndexedDataMap.hxx>


typedef NCollection_IndexedDataMap <TopoDS_Shape, TopTools_IndexedMapOfShape, TopTools_ShapeMapHasher> GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape;

#undef _NCollection_MapHasher


#endif

