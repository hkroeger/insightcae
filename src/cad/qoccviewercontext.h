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

  Handle_V3d_Viewer		myViewer;
  Handle_AIS_InteractiveContext	myContext;
  bool showGrid;
  Aspect_GridType		myGridType;
  Aspect_GridDrawMode		myGridMode;
  Quantity_NameOfColor		myGridColor;
  Quantity_NameOfColor		myGridTenthColor;

};


#endif // QOCCVIEWERCONTEXT_H
