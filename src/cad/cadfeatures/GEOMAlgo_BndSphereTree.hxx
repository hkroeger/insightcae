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

// File:	GEOMAlgo_BndSphereTree.hxx
// Created:	
// Author:	Peter KURNEV
//		<pkv@irinox>
//
#ifndef GEOMAlgo_BndSphereTree_HeaderFile
#define GEOMAlgo_BndSphereTree_HeaderFile

#include <NCollection_UBTree.hxx>
#include <GEOMAlgo_BndSphere.hxx>
#include <TColStd_MapOfInteger.hxx>
#include <TColStd_ListOfInteger.hxx>

typedef NCollection_UBTree <Standard_Integer , GEOMAlgo_BndSphere> GEOMAlgo_BndSphereTree;

  class GEOMAlgo_BndSphereTreeSelector : public GEOMAlgo_BndSphereTree::Selector {
    public:
      Standard_EXPORT GEOMAlgo_BndSphereTreeSelector();
      Standard_EXPORT virtual Standard_Boolean Reject(const GEOMAlgo_BndSphere&) const;
      Standard_EXPORT virtual Standard_Boolean Accept(const Standard_Integer &);
      Standard_EXPORT virtual ~GEOMAlgo_BndSphereTreeSelector();
      
      Standard_EXPORT void Clear();
      Standard_EXPORT void SetBox(const GEOMAlgo_BndSphere&);
      Standard_EXPORT const TColStd_ListOfInteger& Indices() const;

    protected:
      GEOMAlgo_BndSphere  myBox;
      TColStd_MapOfInteger  myFence;
      TColStd_ListOfInteger myIndices;
      
  };

#endif
