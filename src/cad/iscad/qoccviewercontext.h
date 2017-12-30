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

#ifndef QOCCVIEWERCONTEXT_H
#define QOCCVIEWERCONTEXT_H

#include <QtCore>
#include "qocc.h"

class QOCC_DECLSPEC QoccViewerContext : public QObject
{

  Q_OBJECT

public:

  QoccViewerContext();
  ~QoccViewerContext();

  Handle_V3d_Viewer&              getViewer();
  Handle_AIS_InteractiveContext&  getContext();

  Handle_V3d_Viewer createViewer
    (	
     const Standard_CString aDisplay,
     const Standard_ExtString aName,
     const Standard_CString aDomain,
     const Standard_Real ViewSize 
    );
  
  void deleteAllObjects();

  void setGridOffset (Quantity_Length offset);

public slots:

  void toggleGrid ( void );
  void gridXY   ( void );
  void gridXZ   ( void );
  void gridYZ   ( void );
  void gridOn   ( void );
  void gridOff  ( void );
  void gridRect ( void );
  void gridCirc ( void );
  
signals:

  void error (int errorCode, QString& errorDescription);

private:

  Handle(V3d_Viewer)		myViewer;
  Handle(AIS_InteractiveContext)	myContext;
  bool showGrid;
  Aspect_GridType		myGridType;
  Aspect_GridDrawMode		myGridMode;
  Quantity_NameOfColor		myGridColor;
  Quantity_NameOfColor		myGridTenthColor;

};


#endif // QOCCVIEWERCONTEXT_H
