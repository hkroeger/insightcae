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

// File:	GEOMAlgo_BndSphere.hxx
// Created:	
// Author:	Peter KURNEV
//		<pkv@irinox>

#ifndef _GEOMAlgo_BndSphere_HeaderFile
#define _GEOMAlgo_BndSphere_HeaderFile

#include <Standard.hxx>
#include <Standard_Macro.hxx>
#include <gp_Pnt.hxx>
#include <Standard_Real.hxx>
#include <Standard_Boolean.hxx>

//=======================================================================
//class : GEOMAlgo_BndSphere
//purpose  : 
//=======================================================================
class GEOMAlgo_BndSphere  {
 public:
  Standard_EXPORT   
    GEOMAlgo_BndSphere();
  
  Standard_EXPORT 
    virtual ~GEOMAlgo_BndSphere();
  
  void SetCenter(const gp_Pnt& theP) ;
  
  const gp_Pnt& Center() const;
  
  void SetRadius(const Standard_Real theR) ;
  
  Standard_Real Radius() const;
  
  void SetGap(const Standard_Real theGap) ;
  
  Standard_Real Gap() const;
  
  void Add(const GEOMAlgo_BndSphere& theOther) ;
  
  Standard_EXPORT 
    Standard_Boolean IsOut(const GEOMAlgo_BndSphere& theOther) const;
  
  Standard_Real SquareExtent() const;

 protected:
  gp_Pnt myCenter;
  Standard_Real myRadius;
  Standard_Real myGap;
};

#include <GEOMAlgo_BndSphere.lxx>

#endif
