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
#include <QtCore>
#include "qoccinternal.h"
#include "qoccviewercontext.h"

#include <qnamespace.h>

QoccViewerContext::QoccViewerContext()
{
  // Create the OCC Viewers
  TCollection_ExtendedString a3DName("Visual3D");
  myViewer = createViewer( "DISPLAY", a3DName.ToExtString(), "", 1000.0 );
#if (OCC_VERSION_MINOR<6)
  myViewer->Init();
#endif
  myViewer->SetZBufferManagment(Standard_False);
  myViewer->SetDefaultViewProj( V3d_Zpos );	// Top view

  myContext = new AIS_InteractiveContext( myViewer );
  
  showGrid         = false;
  myGridType       = Aspect_GT_Rectangular;
  myGridMode       = Aspect_GDM_Lines;
  myGridColor      = Quantity_NOC_RED4;
  myGridTenthColor = Quantity_NOC_GRAY90;

  myContext->SetHilightColor(Quantity_NOC_WHITE) ;

  setGridOffset (0.0);
  gridXY();
  gridOff();

}


QoccViewerContext::~QoccViewerContext()
{
}

Handle_V3d_Viewer& QoccViewerContext::getViewer()  
{ 
  return myViewer; 
}

Handle_AIS_InteractiveContext& QoccViewerContext::getContext() 
{ 
  return myContext; 
}

Handle_V3d_Viewer QoccViewerContext::createViewer
(
 const Standard_CString aDisplay,
 const Standard_ExtString aName,
 const Standard_CString aDomain,
 const Standard_Real ViewSize 
 )
{
#ifndef WNT
  
#if (OCC_VERSION_MINOR>=6)
  static Handle_Graphic3d_GraphicDriver defaultdevice;
#if (OCC_VERSION_MINOR>6)
  Handle_Aspect_DisplayConnection displayConnection(new Aspect_DisplayConnection());
  defaultdevice = new OpenGl_GraphicDriver( displayConnection );
  Handle_OpenGl_GraphicDriver::DownCast(defaultdevice)->ChangeOptions().ffpEnable=false; // fix to make clip planes work
#else
  defaultdevice = new OpenGl_GraphicDriver(getenv(aDisplay));
#endif
#else
  static Handle(Graphic3d_GraphicDevice) defaultdevice;
  
//  if( defaultdevice.IsNull() )
//    {
      defaultdevice = new Graphic3d_GraphicDevice( getenv(aDisplay) );
//    }
#endif
  return new V3d_Viewer
    (	
     defaultdevice,
     aName,
     aDomain,
     ViewSize,
     V3d_XposYnegZpos,
//       Quantity_NOC_WHITE,
     Quantity_NOC_BLACK,
     V3d_ZBUFFER,
     V3d_GOURAUD,
     V3d_WAIT 
    );
#else
  static Handle( Graphic3d_WNTGraphicDevice ) defaultdevice;
  if( defaultdevice.IsNull() )
    {
      defaultdevice = new Graphic3d_WNTGraphicDevice();
    }

  return new V3d_Viewer
    (	
     defaultdevice,
     aName,
     aDomain,
     ViewSize,
     V3d_XposYnegZpos,
//      Quantity_NOC_WHITE,
     Quantity_NOC_BLACK,
     V3d_ZBUFFER,
     V3d_GOURAUD,
     V3d_WAIT 
    );
#endif  // WNT
}

/*! 
\brief	Deletes all objects.

		This function deletes all dispayed objects from the AIS context.
		No parameters.
*/
void QoccViewerContext::deleteAllObjects()
{
  AIS_ListOfInteractive aList;
  myContext->DisplayedObjects( aList );
  AIS_ListIteratorOfListOfInteractive aListIterator;
  for ( aListIterator.Initialize( aList ); aListIterator.More(); aListIterator.Next() )
  {
    myContext->Remove( aListIterator.Value(), Standard_False);
  }
}
/*! 
\brief	Sets the privileged plane to the XY Axis.  
*/
void QoccViewerContext::gridXY  ( void )
{
  myGridColor      = Quantity_NOC_RED4;
  myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
  gp_Ax3 aPlane(gp_Pnt( 0., 0., 0. ),gp_Dir(0., 0., 1.));
  myViewer->SetPrivilegedPlane( aPlane );
}
/*!
\brief	Sets the privileged plane to the XZ Axis.

		Note the negative direction of the Y axis.
		This is corrrect for a right-handed co-ordinate set.
*/
void QoccViewerContext::gridXZ  ( void )
{
  myGridColor      = Quantity_NOC_BLUE4;
  myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
  gp_Ax3 aPlane( gp_Pnt(0., 0., 0.),gp_Dir(0., -1., 0.) );
  myViewer->SetPrivilegedPlane( aPlane );
}
/*! 
\brief	Sets the privileged plane to the XY Axis.
 */
void QoccViewerContext::gridYZ  ( void )
{
  myGridColor      = Quantity_NOC_GREEN4;
  myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
  gp_Ax3 aPlane( gp_Pnt( 0., 0., 0.), gp_Dir( 1., 0., 0. ) );
  myViewer->SetPrivilegedPlane( aPlane );
}

/*!
\brief  switch the grid on/off.
 */
void QoccViewerContext::toggleGrid  ( void )
{
  if (showGrid){
    showGrid = false;
    if (myGridType == Aspect_GT_Rectangular){
      myGridType = Aspect_GT_Circular;
    } else {
      myGridType = Aspect_GT_Rectangular;
    }
    myViewer->DeactivateGrid();
    myViewer->SetGridEcho( Standard_False );
  } else {
    showGrid = true;
    myViewer->ActivateGrid( myGridType , myGridMode );
    myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
    myViewer->SetGridEcho ( Standard_True );
  }
}



/*!
\brief	Turn the grid on.
 */
void QoccViewerContext::gridOn  ( void )
{
  myViewer->ActivateGrid( myGridType , myGridMode );
  myViewer->SetGridEcho ( Standard_True );
}

/*! 
\brief	Turn the grid off.
*/
void QoccViewerContext::gridOff ( void )
{
  myViewer->DeactivateGrid();
  myViewer->SetGridEcho( Standard_False );
}

/*!
\brief	Select a rectangular grid
*/
void QoccViewerContext::gridRect ( void )
{
  myGridType = Aspect_GT_Rectangular;
  myViewer->ActivateGrid( myGridType , myGridMode );
  myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
}
/*! 
\brief	Select a circular grid.
*/
void QoccViewerContext::gridCirc ( void )
{
  myGridType = Aspect_GT_Circular;
  myViewer->ActivateGrid( myGridType , myGridMode );
  myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
}

void QoccViewerContext::setGridOffset (Quantity_Length offset)
{
  Quantity_Length radius;
  Quantity_Length xSize, ySize;
  Quantity_Length oldOffset;
  
  myViewer->CircularGridGraphicValues( radius, oldOffset );
  myViewer->SetCircularGridGraphicValues( radius, offset);
  
  myViewer->RectangularGridGraphicValues(xSize, ySize, oldOffset);
  myViewer->SetRectangularGridGraphicValues(xSize, ySize, offset);
}

