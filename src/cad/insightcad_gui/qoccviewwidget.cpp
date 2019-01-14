



#include <cmath>
#include <iostream>

#define QT_NO_OPENGL
#include <QtCore>
#include <QtGui>
#include <QMenu>
#include <QAction>


#include "qoccinternal.h"
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_PositionalLight.hxx>
#include "Graphic3d_AspectFillArea3d.hxx"
#include "AIS_Plane.hxx"
#include "AIS_Point.hxx"

#include "occtools.h"
#include "pointertransient.h"


#include "cadtypes.h"
#include "cadpostprocactions.h"

#include "qoccviewwidget.h"
#include "qmodeltree.h"
#include "qdatumitem.h"
#include "datum.h"



#include <QColorDialog>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

QoccViewWidget::QoccViewWidget
( 
 QWidget *parent,
 const Handle_AIS_InteractiveContext& aContext, 
 Qt::WindowFlags f 
)
  : QWidget( parent, f | Qt::MSWindowsOwnDC ),
    myView              ( NULL ),
    myViewResized       ( Standard_False ),
    myViewInitialized   ( Standard_False ),
    myMode              ( CurAction3d_Undefined ),
    myGridSnap          ( Standard_False ),
    myDetection         ( AIS_SOD_Nothing ),
    myRubberBand        ( NULL ),
    myPrecision		( 0.0001 ),
    myViewPrecision     ( 0.0 ),
    myKeyboardFlags     ( Qt::NoModifier ),
    myButtonFlags	( Qt::NoButton ),
    showGrid            ( false ),
    cimode_             ( CIM_Normal )
{
  if (!aContext.IsNull())
  {
    myContext_ = aContext;
  }
  else
  {
    myContextObj_ = new QoccViewerContext(this);
    myContext_ = myContextObj_->getContext();
  }
  // Needed to generate mouse events
  setMouseTracking( true );
  
  // Avoid Qt background clears to improve resizing speed,
  // along with a couple of other attributes
  setAutoFillBackground( false );				
  setAttribute( Qt::WA_NoSystemBackground );
//  setWindowFlags( windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint );

  // This next attribute seems to be the secret of allowing OCC on Win32
  // to "own" the window, even though its only supposed to work on X11.
  setAttribute( Qt::WA_PaintOnScreen );
  
  // Here's a modified pick point cursor from AutoQ3D
  QBitmap curb1( 48, 48 );
  QBitmap curb2( 48, 48 );
  curb1.fill( QColor( 255, 255, 255 ) );
  curb2.fill( QColor( 255, 255, 255 ) );
  QPainter p;
  
  p.begin( &curb1 );
  p.drawLine( 24,  0, 24, 47 );
  p.drawLine(  0, 24, 47, 24 );
  p.setBrush( Qt::NoBrush );
  p.drawRect( 18, 18, 12, 12 );
  p.end();
  myCrossCursor = QCursor( curb2, curb1, 24, 24 );
  
  // Create a rubber band box for later mouse activity
  myRubberBand = new QRubberBand( QRubberBand::Rectangle, this );
//  if (myRubberBand)
//    {
//      // If you don't set a style, QRubberBand doesn't work properly
//      // take this line out if you don't believe me.
//      myRubberBand->setStyle( (QStyle*) new QPlastiqueStyle() );
//    }

  connect
      (
        this, &QoccViewWidget::graphicalSelectionChanged,
        this, &QoccViewWidget::onGraphicalSelectionChanged
      );
}

/*!
\brief	Default destructor for QoccViewWidget.
		This should delete any memory and release any resources. No parameters
		required.
*/
QoccViewWidget::~QoccViewWidget()
{
 if ( myRubberBand )
    {
      delete myRubberBand;
    }
}

/*!
\brief	The initializeOCC() procedure.
		This function creates the widget's view using the interactive context
		provided. Currently it also creates a trihedron object in the lower left
		corner - this will eventually be managed by an external system setting.
\param	aContext Handle to the AIS Interactive Context managing the view
\return	nothing
*/
void QoccViewWidget::initializeOCC(/*const Handle_AIS_InteractiveContext& aContext*/)
{
  Aspect_RenderingContext rc = 0;

//  myContext = aContext;

  myViewer  = myContext_->CurrentViewer();
  myView    = myViewer->CreateView();
  
  int windowHandle = (int) winId();
  short lo = (short)   windowHandle;
  short hi = (short) ( windowHandle >> 16 );
  
#ifdef WNT
  // rc = (Aspect_RenderingContext) wglGetCurrentContext();
  myWindow = new WNT_Window
    (
     Handle(Graphic3d_WNTGraphicDevice)::DownCast( myContext->CurrentViewer()->Device() ) ,
     (int) hi, (int) lo 
     );
  // Turn off background erasing in OCC's window
  myWindow->SetFlags( WDF_NOERASEBKGRND );
#else
  // rc = (Aspect_RenderingContext) glXGetCurrentContext(); // Untested!
  myWindow = new Xw_Window
    (
#if ((OCC_VERSION_MAJOR>=7)||(OCC_VERSION_MINOR>=6))
      myContext_->CurrentViewer()->Driver()->GetDisplayConnection(), windowHandle
#else
     Handle_Graphic3d_GraphicDevice::DownCast( myContext->CurrentViewer()->Device() ),
     (int) hi, (int) lo, Xw_WQ_SAMEQUALITY, Quantity_NOC_BLACK 
#endif
    );
#endif // WNT
	
  if (!myView.IsNull())
    {

      // Set my window (Hwnd) into the OCC view
      myView->SetWindow( myWindow, rc
#if (OCC_VERSION_MAJOR<7)
                         , paintCallBack, this  
#endif
                       );
      // Set up axes (Trihedron) in lower left corner.
      myView->SetScale( 2 );			// Choose a "nicer" intial scale
      
      // Set up axes (Trihedron) in lower left corner.
#ifdef OCC_PATCHED
      myView->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_BLACK, 0.1, V3d_ZBUFFER );
#else
      myView->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_BLACK, 0.1, V3d_WIREFRAME );
#endif
      // For testing OCC patches
      // myView->ColorScaleDisplay();	
      // Map the window
      if (!myWindow->IsMapped())
	{
	  myWindow->Map();
	}
      // Force a redraw to the new window on next paint event
      myViewResized = Standard_True;
      // Set default cursor as a cross
      setMode( CurAction3d_Nothing );
      // This is to signal any connected slots that the view is ready.
      myViewInitialized = Standard_True;

#if (OCC_VERSION_MAJOR>=7)
    myView->SetShadingModel(V3d_PHONG);
#endif

#if (OCC_VERSION_MAJOR<7)
      myView->EnableGLLight(false);
#endif
//       myViewer->InitActiveLights();
//       while ( myViewer->MoreActiveLights ()) {
//         Handle_V3d_Light myLight = myViewer->ActiveLight();
//         myViewer->SetLightOff(myLight);
//         myViewer->NextActiveLights();
//       }
//       myViewer->SetDefaultLights();
      
Handle(V3d_AmbientLight)     L1 = new V3d_AmbientLight(myViewer, Quantity_NOC_WHITE);
//Handle(V3d_Light) La = new V3d_PositionalLight(myViewer,10000,-3000,30000,Quantity_NOC_WHITE, 0.8, 0);
//Handle(V3d_Light) Lb = new V3d_PositionalLight(myViewer,10000,-3000,-30000,Quantity_NOC_WHITE, 0.8, 0);
//Handle(V3d_Light) Lc = new V3d_PositionalLight(myViewer,-30000,-3000,-10000,Quantity_NOC_WHITE, 0.8, 0);
Handle(V3d_DirectionalLight) L2 = new V3d_DirectionalLight(myViewer,V3d_XnegYnegZneg, Quantity_NOC_WHITE, true);

    //       myView->SetLightOn(new V3d_PositionalLight (myViewer,  10000,-3000,30000,  Quantity_NOC_ANTIQUEWHITE3, 0.8, 0));
//       myView->SetLightOn(new V3d_PositionalLight (myViewer,  10000,-3000,-30000,  Quantity_NOC_ANTIQUEWHITE3, 0.8, 0));
//       myView->SetLightOn(new V3d_PositionalLight (myViewer,-30000,-3000,-10000,  Quantity_NOC_ANTIQUEWHITE3, 0.8, 0));
//       myView->UpdateLights();
      myViewer->SetLightOn();

      //Handle_V3d_Light myDirectionalLight = new V3d_DirectionalLight( myViewer, 0,0,0, 1,-0.3,0.5 , Quantity_NOC_WHITE, Standard_True );//, V3d_TypeOfOrientation(-1, 0,0), Quantity_NOC_WHITE, Standard_False);
      //myView->SetLightOn(myDirectionalLight);

      myView->SetProj( V3d_Zpos );
      myView->SetUp( V3d_Xpos );
      fitAll();

      emit initialized();
    }
}

/*!
  \brief	Returns a NULL QPaintEngine
  This should result in a minor performance benefit.
*/
QPaintEngine* QoccViewWidget::paintEngine() const
{
  return NULL;
}

/*!
\brief	Paint Event
		Called when the Widget needs to repaint itself
\param	e The event data.
*/
void QoccViewWidget::paintEvent ( QPaintEvent * /* e */)
{
  if ( !myViewInitialized )
    {
      if ( winId() )
	{
	  initializeOCC( /*myContext*/ );
	}
    }
  if ( !myViewer.IsNull() )
    {
      redraw( true );	
    }
}

/*!
\brief	Resize event.
		Called when the Widget needs to resize itself, but seeing as a paint
		event always follows a resize event, we'll move the work into the
		paint event.
\param	e The event data.
*/
void QoccViewWidget::resizeEvent ( QResizeEvent * /* e */ )
{
  myViewResized = Standard_True;
}	




/*!
\brief	Mouse press event
\param	e The event data.
*/
void QoccViewWidget::mousePressEvent( QMouseEvent* e )
{
  myButtonFlags = e->button();
  
  // Cache the keyboard flags for the whole gesture
  myKeyboardFlags = e->modifiers();
  
  // The button mappings can be used as a mask. This code prevents conflicts
  // when more than one button pressed simutaneously.
  if ( e->button() & Qt::LeftButton )
    {
      onLeftButtonDown  ( myKeyboardFlags, e->pos() );
    }
  else if ( e->button() & Qt::RightButton )
    {
      onRightButtonDown ( myKeyboardFlags, e->pos() );
    }
  else if ( e->button() & Qt::MidButton )
    {
      onMiddleButtonDown( myKeyboardFlags, e->pos() );
    }
}




/*!
\brief	Mouse release event
\param	e The event data.
*/
void QoccViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  myButtonFlags = Qt::NoButton;
  redraw();							// Clears up screen when menu selected but not used.
  hideRubberBand();
  if ( e->button() & Qt::LeftButton )
    {
      onLeftButtonUp  ( myKeyboardFlags, e->pos() );
    }
  else if ( e->button() & Qt::RightButton )
    {
      onRightButtonUp ( myKeyboardFlags, e->pos() );
    }
  else if ( e->button() & Qt::MidButton )
    {
      onMiddleButtonUp( myKeyboardFlags, e->pos() );
    }
}




/*!
\brief	Mouse move event, driven from application message loop
\param	e The event data.
*/
void QoccViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  Standard_Real X, Y, Z;
  
  myCurrentPoint = e->pos();
  //Check if the grid is active and that we're snapping to it
  if( myContext_->CurrentViewer()->Grid()->IsActive() && myGridSnap )
    {
      myView->ConvertToGrid
	( 
	 myCurrentPoint.x(),
	 myCurrentPoint.y(),
	 myV3dX,
	 myV3dY,
	 myV3dZ 
	);
      emit mouseMoved( myV3dX, myV3dY, myV3dZ );
    }
  else //	this is the standard case
    {
      if (convertToPlane
	  ( 
	   myCurrentPoint.x(),
	   myCurrentPoint.y(),
	   X, Y, Z 
	  ) )
	{
	  myV3dX = precision( X );
	  myV3dY = precision( Y );
	  myV3dZ = precision( Z );
	  emit mouseMoved( myV3dX, myV3dY, myV3dZ );
	}
      else
	{
	  emit sendStatus ( tr("Indeterminate Point") );
	}
    }
  
  onMouseMove( e->buttons(), myKeyboardFlags, e->pos(), e->modifiers() );
}




/*!
  \brief	A leave event is sent to the widget when the mouse cursor leaves
  the widget.
  This sub-classed event handler fixes redraws when gestures are interrupted
  by use of parent menus etc. (Candidate for change)
  \param	e	The event data
*/
void QoccViewWidget::leaveEvent ( QEvent* /* e */ )
{
  myButtonFlags = Qt::NoButton;
}



void QoccViewWidget::displayContextMenu( const QPoint& p)
{
  if (QModelTreeItem* mi=dynamic_cast<QModelTreeItem*>(getSelectedItem()))
  {
      // an item exists under the requested position
      mi->showContextMenu(mapToGlobal(p));
  }
  else
  {
    // display view menu
    QMenu vmenu;
    QAction *a;

    a = new QAction(("Fit &all"), this);
    vmenu.addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::fitAll);


    QMenu* directionmenu=vmenu.addMenu("Standard views");
    directionmenu->addAction( a = new QAction(("+X"), this) );
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::viewRight);
    directionmenu->addAction( a = new QAction(("-X"), this) );
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::viewLeft);

    directionmenu->addAction( a = new QAction(("+Y"), this) );
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::viewBack);
    directionmenu->addAction( a = new QAction(("-Y"), this) );
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::viewFront);

    directionmenu->addAction( a = new QAction(("+Z"), this) );
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::viewTop);
    directionmenu->addAction( a = new QAction(("-Z"), this) );
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::viewBottom);

    a = new QAction(("Toggle &grid"), this);
    vmenu.addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::toggleGrid);

    a = new QAction(("Toggle clip plane (&XY)"), this);
    vmenu.addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::toggleClipXY);
    a = new QAction(("Toggle clip plane (&YZ)"), this);
    vmenu.addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::toggleClipYZ);
    a = new QAction(("Toggle clip plane (X&Z)"), this);
    vmenu.addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::toggleClipXZ);

    a = new QAction(("Change background color..."), this);
    vmenu.addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::background);
//    a = new QAction(("Display all &shaded"), this);
//    vmenu.addAction(a);
//    connect(a, &QAction::triggered,
//            this, &QModelTree::allShaded);
//    a = new QAction(("Display all &wireframe"), this);
//    vmenu.addAction(a);
//    connect(a, &QAction::triggered,
//            this, &QModelTree::allWireframe);
//    vmenu.addAction(a);
//    a = new QAction(("&Reset shading and visibility"), this);
//    connect(a, &QAction::triggered,
//            me->modeltree(), &QModelTree::resetViz);

//    QMenu *clipplanemenu_=vmenu.addMenu("Clip at datum plane");
//    me->model()->populateClipPlaneMenu(clipplanemenu_, me->viewer());


    QMenu *msmenu=vmenu.addMenu("Measure");

    a=new QAction("Distance between points", this);
    msmenu->addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::onMeasureDistance);

    a=new QAction("Select vertices", this);
    msmenu->addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::onSelectPoints);

    a=new QAction("Select edges", this);
    msmenu->addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::onSelectEdges);

    a=new QAction("Select faces", this);
    msmenu->addAction(a);
    connect(a, &QAction::triggered,
            this, &QoccViewWidget::onSelectFaces);

    vmenu.exec(mapToGlobal(p));
  }
}




/*!
\brief	The QWheelEvent class contains parameters that describe a wheel event. 
*/
void QoccViewWidget::wheelEvent ( QWheelEvent* e )
{
  if ( !myView.IsNull() )
    {
      Standard_Real currentScale = myView->Scale();
      if (e->delta() > 0)
	{
	  currentScale *= 1.10; // +10%
	}
      else
	{
	  currentScale /= 1.10; // -10%
	}
      myView->SetScale( currentScale );
    }
  else
    {
      e->ignore();
    }
}




void QoccViewWidget::keyPressEvent(QKeyEvent* e)
{
//   std::cout<<e->modifiers()<<std::endl;
    if ( ( e->modifiers() & ZOOMSHORTCUTKEY ) && (myMode == CurAction3d_Nothing) )
    {
      setMode(CurAction3d_DynamicZooming);
    }  
    else if ( ( e->modifiers() & PANSHORTCUTKEY ) && (myMode == CurAction3d_Nothing) )
    {
      setMode(CurAction3d_DynamicPanning);
    }  
    else if ( ( e->modifiers() & ROTATESHORTCUTKEY ) && (myMode == CurAction3d_Nothing) )
    {
      setMode(CurAction3d_DynamicRotation);
    }
    else
      QWidget::keyPressEvent(e);
}




void QoccViewWidget::keyReleaseEvent(QKeyEvent* e)
{
//   std::cout<<e->modifiers()<<std::endl;
    if ( !( e->modifiers() & ZOOMSHORTCUTKEY ) && (myMode == CurAction3d_DynamicZooming) )
    {
      setMode(CurAction3d_Nothing);
    }  
    else if ( !( e->modifiers() & PANSHORTCUTKEY ) && (myMode == CurAction3d_DynamicPanning) )
    {
      setMode(CurAction3d_Nothing);
    }  
    else if ( !( e->modifiers() & ROTATESHORTCUTKEY ) && (myMode == CurAction3d_DynamicRotation) )
    {
      setMode(CurAction3d_Nothing);
    }
    else
      QWidget::keyReleaseEvent(e);
}




void QoccViewWidget::onGraphicalSelectionChanged(QDisplayableModelTreeItem* selection, QoccViewWidget* viewer)
{
    // Remove previously displayed sub objects from display
    for (Handle_AIS_InteractiveObject& o: additionalDisplayObjectsForSelection_)
    {
        getContext()->Erase(o, false);
    }
    additionalDisplayObjectsForSelection_.clear();

    // Display sub objects for current selection
    if (QFeatureItem* ms = dynamic_cast<QFeatureItem*>(selection))
    {
        insight::cad::Feature& sm=ms->solidmodel();

        const insight::cad::Feature::RefPointsList& pts=sm.getDatumPoints();

        // reverse storage to detect collocated points
        typedef std::map<arma::mat, std::string, insight::compareArmaMat> trpts;
        trpts rpts;
        for (const insight::cad::Feature::RefPointsList::value_type& p: pts)
        {
            const std::string& name=p.first;
            const arma::mat& xyz=p.second;
//             std::cout<<name<<":"<<xyz<<std::endl;

            trpts::iterator j=rpts.find(xyz);
            if (j!=rpts.end())
            {
                j->second = j->second+"="+name;
            }
            else
            {
                rpts[xyz]=name;
            }
        }

        for (const trpts::value_type& p: rpts)
        {
            const std::string& name=p.second;
            const arma::mat& xyz=p.first;
            Handle_AIS_InteractiveObject o
            (
                new insight::cad::InteractiveText(name, xyz)
            );
            additionalDisplayObjectsForSelection_.push_back(o);
            getContext()->Display(o, false);
        }
    }

    getContext()->UpdateCurrentViewer();
}


/*!
  \brief	Go idle
  This called from various locations, and also exposed as a slot.
*/
void QoccViewWidget::idle( )
{
  setMode( CurAction3d_Nothing );
}
/*!
  \brief	The main redraw function
  This called from various locations.
*/
void QoccViewWidget::redraw( bool isPainting )
{
  if ( !myView.IsNull() )					// Defensive test.
    {
      if ( myViewResized )
	{
	  myView->MustBeResized();
	  viewPrecision( true );
	}
      else
	{	
	  // Don't repaint if we are already redrawing
	  // elsewhere due to a keypress or mouse gesture
	  if ( !isPainting || ( isPainting && myButtonFlags == Qt::NoButton ) )	
	    {												
	      myView->Redraw();
	    }
	}
    }
  myViewResized = Standard_False;
}


QDisplayableModelTreeItem* QoccViewWidget::getOwnerItem(Handle_AIS_InteractiveObject selected)
{
  Handle_Standard_Transient own=selected->GetOwner();
  if (!own.IsNull())
  {
      if (PointerTransient *smo=dynamic_cast<PointerTransient*>(own
#if (OCC_VERSION_MAJOR<7)
              .Access()
#else
              .get()
#endif
      ))
      {
          if (QDisplayableModelTreeItem* mi=dynamic_cast<QDisplayableModelTreeItem*>(smo->getPointer()))
          {
              return mi;
          }
      }
  }

  return NULL;
}

QDisplayableModelTreeItem* QoccViewWidget::getSelectedItem()
{
  if (myContext_->HasDetected())
  {
      if (myContext_->DetectedInteractive()->HasOwner())
      {
          return getOwnerItem(myContext_->DetectedInteractive());
      }
  }

  return NULL;
}


void QoccViewWidget::connectModelTree(QModelTree* mt) const
{
  connect(mt, &QModelTree::show,
          this, &QoccViewWidget::onShow);
  connect(mt, &QModelTree::hide,
          this, &QoccViewWidget::onHide);
  connect(mt, &QModelTree::setDisplayMode,
          this, &QoccViewWidget::onSetDisplayMode);
  connect(mt, &QModelTree::setColor,
          this, &QoccViewWidget::onSetColor);
  connect(mt, &QModelTree::setResolution,
          this, &QoccViewWidget::onSetResolution);
}

QSize QoccViewWidget::sizeHint() const
{
  QSize s;
  s.setWidth(1024);
  s.setHeight(768);
  return s;
}

/*!
\brief	Just fits the current window
		This function just fits the current objects to the window, without
		either reducing or increasing the Z extents. This can cause clipping
		of the objects after rotation.
\return	Nothing
*/
void QoccViewWidget::fitExtents( void )
{
  if (!myView.IsNull())
    {
      myView->FitAll();
      viewPrecision( true );
    }
}

/*!
\brief	Fits the model to view extents
		This function fits the current objects to the window,  
		reducing or increasing the Z extents as needed. 
\return	Nothing
*/
void QoccViewWidget::fitAll( void )
{
  if (!myView.IsNull())

    {
      myView->ZFitAll();
      myView->FitAll();
      viewPrecision( true );
    }
}

//-----------------------------------------------------------------------------
/*!
\brief	Sets up the view for a rubberband window zoom
\return	Nothing
*/
void QoccViewWidget::fitArea( void )
{
  setMode( CurAction3d_WindowZooming );
}

/*!
\brief	Sets up the view for a dynamic zoom
\return	Nothing
*/
void QoccViewWidget::zoom( void )
{
  setMode( CurAction3d_DynamicZooming );
}

/*!
\brief	Sets up the view for panning
\return	Nothing
*/
void QoccViewWidget::pan( void )
{
  setMode( CurAction3d_DynamicPanning );
}

/*!
\brief	Sets up the view for dynamic rotation
\return	Nothing
*/
void QoccViewWidget::rotation( void )
{
  setMode( CurAction3d_DynamicRotation );
}

/*!
\brief	Sets up the view for global panning, whatever that is!
\return	Nothing
*/
void QoccViewWidget::globalPan()
{
  if (!myView.IsNull())
    {
      // save the current zoom value
      myCurZoom = myView->Scale();
      // Do a Global Zoom
      myView->FitAll();
      viewPrecision( true );
      // Set the mode
      setMode( CurAction3d_GlobalPanning );
    }
}

/*!
\brief	This aligns the view to the current privilege plane.
		Not so interesting at the moment, but wait until custom
		grid planes are implemented!
\return	Nothing
*/
void QoccViewWidget::viewGrid()
{
  if(!myView.IsNull())
    {
      myView->SetFront();
    }
}

/*!
\brief	View from canonical "front".
\return	Nothing
*/
void QoccViewWidget::viewFront()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_Yneg );
    }
}
/*!
\brief	View from canonical "back".
\return	Nothing
*/
void QoccViewWidget::viewBack()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_Ypos );
    }
}
/*!
\brief	View from canonical "top".
		This is traditional XOY axis.
\return	Nothing
*/
void QoccViewWidget::viewTop()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_Zpos );
    }
}

/*!
\brief	View from canonical "bottom".
\return	Nothing
*/
void QoccViewWidget::viewBottom()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_Zneg );
    }
}
/*!
\brief	View from canonical "left".
\return	Nothing
*/
void QoccViewWidget::viewLeft()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_Xneg );
    }
}
/*!
\brief	View from canonical "right".
\return	Nothing
*/
void QoccViewWidget::viewRight()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_Xpos );
    }
}
/*!
  \brief	View using axonometric projection.
  \return	Nothing
*/

void QoccViewWidget::viewAxo()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_XnegYnegZpos );
    }
}

void QoccViewWidget::viewTopFront()
{
  if(!myView.IsNull())
    {
      myView->SetProj( V3d_YnegZpos );
    }
}


void QoccViewWidget::viewReset()
{
  if(!myView.IsNull())
    {
      myView->Reset();
    }
}

void QoccViewWidget::hiddenLineOff()
{
  if(!myView.IsNull())
    {
      QGuiApplication::setOverrideCursor( Qt::WaitCursor );
      myView->SetComputedMode( Standard_False );
      QGuiApplication::restoreOverrideCursor();
    }
}

void QoccViewWidget::hiddenLineOn()
{
  if(!myView.IsNull())
    {
      QGuiApplication::setOverrideCursor( Qt::WaitCursor );
      myView->SetComputedMode( Standard_True );
      QGuiApplication::restoreOverrideCursor();
    }
}

void QoccViewWidget::background()
{
  
  QColor aColor ;
  Standard_Real R1;
  Standard_Real G1;
  Standard_Real B1;
  myView->BackgroundColor(Quantity_TOC_RGB,R1,G1,B1);
  aColor.setRgb(( int )R1*255, ( int )G1*255, ( int )B1*255);

  QColor aRetColor = QColorDialog::getColor(aColor);

  if( aRetColor.isValid() )
    {
      R1 = aRetColor.red()/255.;
      G1 = aRetColor.green()/255.;
      B1 = aRetColor.blue()/255.;
//       myView->SetBgGradientStyle( Aspect_GFM_NONE ); // cancel gradient background
//       myView->SetBgImageStyle( Aspect_FM_NONE ); 
//       Quantity_Color qw(R1,G1,B1,Quantity_TOC_RGB);
//       myView->SetBgGradientColors( qw, qw, Aspect_GFM_NONE );
      myView->SetBackgroundColor(Quantity_TOC_RGB,R1,G1,B1);
      myView->Update();
    }
  redraw();
}

void QoccViewWidget::setReset ()
{
  if(!myView.IsNull())
    {
      myView->SetViewOrientationDefault() ;
      viewPrecision( true );
    }
}


void QoccViewWidget::toggleClipXY(void)
{
    toggleClip( 0,0,0, 0,0,1 );
}

void QoccViewWidget::toggleClipXZ(void)
{
    toggleClip( 0,0,0, 0,1,0 );
}

void QoccViewWidget::toggleClipYZ(void)
{
    toggleClip( 0,0,0, 1,0,0 );
}

/*!
\brief  switch the grid on/off.
 */
void QoccViewWidget::toggleGrid  ( void )
{
  Aspect_GridType		myGridType= Aspect_GT_Rectangular;
  Aspect_GridDrawMode		myGridMode=Aspect_GDM_Lines;
  Quantity_NameOfColor	myGridColor = Quantity_NOC_RED4;
  Quantity_NameOfColor	myGridTenthColor=Quantity_NOC_GRAY90;

  if (showGrid){
    showGrid = false;
    myViewer->DeactivateGrid();
    myViewer->SetGridEcho( Standard_False );
  } else {
    showGrid = true;
    myViewer->ActivateGrid( Aspect_GT_Rectangular , myGridMode );
    myViewer->Grid()->SetColors( myGridColor, myGridTenthColor );
    myViewer->SetGridEcho ( Standard_True );
  }
}

void QoccViewWidget::toggleClip(double px, double py, double pz, double nx, double ny, double nz)
{
#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<7))
  if (clipPlane_.IsNull())
  {
    gp_Pln pl( gp_Pnt(px,py,pz), gp_Dir(nx,ny,nz) );
    double a, b, c, d;
    pl.Coefficients(a, b, c, d);
    clipPlane_=new V3d_Plane(
//       myViewer, 
      a, b, c, d);
    myView->SetPlaneOn(clipPlane_);
  }
  else
  {
    myView->SetPlaneOff();
    clipPlane_.Nullify();
  }
#else
  if (clipPlane_.IsNull())
  {
    gp_Pln pl( gp_Pnt(px,py,pz), gp_Dir(nx,ny,nz) );
    clipPlane_=new Graphic3d_ClipPlane(pl);
    Graphic3d_MaterialAspect mat(Graphic3d_NOM_DEFAULT);
    mat.SetColor(Quantity_Color(Quantity_NOC_WHITE));
    clipPlane_->SetCapping(true);
    clipPlane_->SetCappingMaterial(mat);
    clipPlane_->SetOn(true);
//     clipPlane_->SetCappingHatchOn();
//     clipPlane_->SetCappingHatch(Aspect_HS_DIAGONAL_45_WIDE);

//     Handle_Graphic3d_AspectFillArea3d ca=clipPlane_->CappingAspect();
//     ca->SetEdgeOn();

    myView->AddClipPlane(clipPlane_);
    myView->Redraw();
  }
  else
  {
    myView->RemoveClipPlane(clipPlane_);
    clipPlane_.Nullify();
  }
#endif
}


void QoccViewWidget::displayMessage(const QString& msg)
{
}


Bnd_Box QoccViewWidget::sceneBoundingBox() const
{
    AIS_ListOfInteractive loi;
    myContext_->DisplayedObjects(loi);

    Bnd_Box bbb;
    for (AIS_ListOfInteractive::const_iterator i=loi.cbegin(); i!=loi.cend(); i++)
      {
          Handle_AIS_InteractiveObject o = *i;
          Handle_AIS_Shape a = Handle_AIS_Shape::DownCast(o);
          if (!a.IsNull())
            {
              Bnd_Box bb;
              o->BoundingBox(bb);
              bbb.Add(bb);
            }
      }

    return bbb;
}

bool QoccViewWidget::updatePlaneSize(const Handle_AIS_InteractiveObject& ppl, double size)
{

    bool changed=false;
    Handle_AIS_Plane pl = Handle_AIS_Plane::DownCast(ppl);
    if (!pl.IsNull())
    {
//        double x, y;
//        pl->Size(x, y);
//        std::cout<<"PLANE! "<<x<<" "<<y<<std::endl;
//        std::cout<<"size="<<size<<std::endl;
        pl->SetSize(size, size);
//        pl->SetToUpdate();
        pl->Redisplay(true);
        changed=true;
    }

    const PrsMgr_ListOfPresentableObjects& children = ppl->Children();
    if (children.Extent()>0)
    {
//        std::cout<<"checking "<<children.Extent()<<" children"<<std::endl;
        for (PrsMgr_ListOfPresentableObjects::const_iterator i=children.begin(); i!=children.end(); i++)
        {
    //        Handle_AIS_InteractiveObject pplc = Handle_AIS_InteractiveObject::DownCast(*i);
    //        if (!pplc.IsNull()) updatePlaneSize(pplc, size);
            Handle_AIS_ConnectedInteractive pplc2 = Handle_AIS_ConnectedInteractive::DownCast(*i);
            if (!pplc2.IsNull()) changed|=updatePlaneSize(pplc2->ConnectedTo(), size);
        }
    }

    return changed;
}

void QoccViewWidget::updatePlanesSizes()
{
    Bnd_Box scenebb = sceneBoundingBox();

    double size=1000;
    if (!scenebb.IsVoid())
    {
        double diag=(scenebb.CornerMax().XYZ()-scenebb.CornerMin().XYZ()).Modulus();
        size=1.2*diag;
    }

    AIS_ListOfInteractive loi;
    getContext()->DisplayedObjects(loi);
    for (AIS_ListOfInteractive::const_iterator i=loi.cbegin(); i!=loi.cend(); i++)
    {
        Handle_AIS_InteractiveObject o=*i;
        updatePlaneSize(o, size);
    }
}

void QoccViewWidget::onShow(QDisplayableModelTreeItem* di)
{
  if (di)
    {
      Handle_AIS_InteractiveObject ais = di->ais( *getContext() );

      getContext()->Display
      (
        ais
#if (OCC_VERSION_MAJOR>=7)
        , false
#endif
      );
      getContext()->SetDisplayMode(ais, di->shadingMode(), Standard_False );
      getContext()->SetColor(ais, di->color(), Standard_True );

      updatePlanesSizes();
    }
}


void QoccViewWidget::onHide(QDisplayableModelTreeItem* di)
{
  if (di)
    {
      Handle_AIS_InteractiveObject ais = di->ais( *getContext() );

      getContext()->Erase
      (
        ais
#if (OCC_VERSION_MAJOR>=7)
        , true
#endif
      );
    }
}


void QoccViewWidget::onSetDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm)
{
  if (di)
    {
      getContext()->SetDisplayMode(di->ais( *getContext() ), sm, Standard_True );
    }
}

void QoccViewWidget::onSetColor(QDisplayableModelTreeItem* di, Quantity_Color c)
{
  if (di)
    {
      getContext()->SetColor(di->ais( *getContext() ), c, Standard_True );
    }
}

void QoccViewWidget::onSetResolution(QDisplayableModelTreeItem* di, double res)
{
  if (di)
    {
      getContext()->SetDeviationCoefficient
          (
            di->ais( *getContext() ),
            res
  #if (OCC_VERSION_MAJOR>=7)
            , true
  #endif
          );
    }
}



void QoccViewWidget::onSetClipPlane(QObject* qdatum)
{
    insight::cad::Datum* datum = reinterpret_cast<insight::cad::Datum*>(qdatum);
    gp_Ax3 pl = datum->plane();
    gp_Pnt p = pl.Location();
    gp_Dir n = pl.Direction();
    toggleClip( p.X(),p.Y(),p.Z(), n.X(),n.Y(),n.Z() );
}


void QoccViewWidget::onMeasureDistance()
{
  measpts_p1_.reset();
  measpts_p2_.reset();
  cimode_=CIM_MeasurePoints;
  getContext()->Activate( AIS_Shape::SelectionMode(TopAbs_VERTEX) );
  emit sendStatus("Please select first point!");
}


void QoccViewWidget::onSelectPoints()
{
  selpts_.reset();
  cimode_=CIM_InsertPointIDs;
  getContext()->Activate( AIS_Shape::SelectionMode(TopAbs_VERTEX) );
  emit sendStatus("Please select points and finish with right click!");
}

void QoccViewWidget::onSelectEdges()
{
  selpts_.reset();
  cimode_=CIM_InsertEdgeIDs;
  getContext()->Activate( AIS_Shape::SelectionMode(TopAbs_EDGE) );
  emit sendStatus("Please select edges and finish with right click!");
}

void QoccViewWidget::onSelectFaces()
{
  selpts_.reset();
  cimode_=CIM_InsertFaceIDs;
  getContext()->Activate( AIS_Shape::SelectionMode(TopAbs_FACE) );
  emit sendStatus("Please select faces and finish with right click!");
}


void QoccViewWidget::onUnfocus()
{
  doUnfocus();
}

void QoccViewWidget::doUnfocus(bool newFocusIntended)
{
  if (focussedObject)
  {
    if (!focussedObject->was_visible)
    {
      getContext()->Erase
      (
        focussedObject->ais
#if (OCC_VERSION_MAJOR>=7)
        , false
#endif
      );
    }

    // make everything else transparent
    AIS_ListOfInteractive loi;
    getContext()->DisplayedObjects(loi);
    for (AIS_ListOfInteractive::const_iterator i=loi.cbegin(); i!=loi.cend(); i++)
    {
        Handle_AIS_InteractiveObject o=*i;
        if (o!=focussedObject->ais)
          getContext()->SetTransparency(o, 0, false);
    }

    if (!newFocusIntended) getContext()->UpdateCurrentViewer();

    focussedObject.reset();
  }
}

void QoccViewWidget::onFocus(Handle_AIS_InteractiveObject ais)
{
  if (focussedObject)
    if (focussedObject->ais == ais) return;

  doUnfocus(true);

  AIS_DisplayStatus s = getContext()->DisplayStatus(ais);

  // make everything else transparent
  AIS_ListOfInteractive loi;
  getContext()->DisplayedObjects(loi);
  for (AIS_ListOfInteractive::const_iterator i=loi.cbegin(); i!=loi.cend(); i++)
  {
      Handle_AIS_InteractiveObject o=*i;
      if (o!=ais)
        getContext()->SetTransparency(o, 0.9, false);
  }

  focussedObject.reset(new FocusObject);
  FocusObject& co = *focussedObject;

  co.ais=ais;
  co.was_visible = (s==AIS_DS_Displayed);

  if (!co.was_visible)
  {
    getContext()->Display
    (
      ais
#if (OCC_VERSION_MAJOR>=7)
      , false
#endif
    );
    getContext()->SetDisplayMode(ais, AIS_Shaded, false );
    getContext()->SetColor(ais, Quantity_NOC_RED, false );
  }

  getContext()->UpdateCurrentViewer();
}

/*!
  \brief	This function handles left button down events from the mouse.
*/
void QoccViewWidget::onLeftButtonDown(  Qt::KeyboardModifiers nFlags, const QPoint point )
{
  myStartPoint = point;

  if ( nFlags & ZOOMSHORTCUTKEY )
    {
      setMode( CurAction3d_DynamicZooming );
    }
  else if ( nFlags & PANSHORTCUTKEY )
    {
      setMode( CurAction3d_DynamicPanning );
    }
  else if ( nFlags & ROTATESHORTCUTKEY )
    {
      setMode( CurAction3d_DynamicRotation );
      myView->StartRotation( point.x(), point.y() );
    }  else
    {
      switch ( myMode )
        {
	case CurAction3d_Nothing:
	  // emit pointClicked( myV3dX, myV3dY, myV3dZ );
	  break;

	case CurAction3d_Picking:
	  break;

	case CurAction3d_DynamicZooming:
	  break;

	case CurAction3d_WindowZooming:
	  break;

	case CurAction3d_DynamicPanning:
	  break;

	case CurAction3d_GlobalPanning:
	  break;

	case CurAction3d_DynamicRotation:
	  myView->StartRotation( myStartPoint.x(), myStartPoint.y() );
	  break;

	default:
	  Standard_Failure::Raise( "Incompatible Current Mode" );
	  break;
        }
    }
}

/*!
  \brief	This function handles middle button down events from the mouse.
*/
void QoccViewWidget::onMiddleButtonDown(  Qt::KeyboardModifiers nFlags, const QPoint point )
{
/*
  myStartPoint = point;
  if ( nFlags & ZOOMSHORTCUTKEY )
    {
      setMode( CurAction3d_DynamicZooming );
    }
  else if ( nFlags & PANSHORTCUTKEY )
    {
      setMode( CurAction3d_DynamicPanning );
    }
  else
    {
      setMode( CurAction3d_DynamicRotation );
      myView->StartRotation( point.x(), point.y() );
    }
*/
}

/*!
  \brief	This function handles right button down events from the mouse.
*/
void QoccViewWidget::onRightButtonDown(  Qt::KeyboardModifiers, const QPoint point )
{
  myStartPoint = point;
  //  else
//    {
//      emit popupMenu ( this, point );
//    }
}

/*!
  \brief	This function handles left button up events from the mouse.
  This marks the end of the gesture.
*/
void QoccViewWidget::onLeftButtonUp(  Qt::KeyboardModifiers nFlags, const QPoint point )
{
  myCurrentPoint = point;
  
  if (
  ( nFlags & ZOOMSHORTCUTKEY )
  ||( nFlags & PANSHORTCUTKEY )
  ||( nFlags & ROTATESHORTCUTKEY )
   )
    {
      // Deactivates dynamic zooming
      setMode( CurAction3d_Nothing );
    }
    else
    {
      switch( myMode )
	{

	case CurAction3d_Nothing:

	  myContext_->Select(true);

	  if (cimode_==CIM_Normal)
	    {
	      emit graphicalSelectionChanged(getSelectedItem(), this);
	    }
	  else if (cimode_==CIM_MeasurePoints)
	    {
	      myContext_->InitSelected();
	      if (myContext_->MoreSelected())
	      {
		TopoDS_Shape v = myContext_->SelectedShape();
//		BRepTools::Dump(v, std::cout);
		gp_Pnt p =BRep_Tool::Pnt(TopoDS::Vertex(v));
		std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;

		if (!measpts_p1_)
		  {
		    measpts_p1_=insight::cad::matconst(insight::vec3(p));
		    emit sendStatus("Please select second point!");
		  }
		else if (!measpts_p2_)
		  {
		    measpts_p2_=insight::cad::matconst(insight::vec3(p));
		    emit sendStatus("Measurement is created...");

		    emit addEvaluationToModel
			(
			  "distance measurement",
			  insight::cad::PostprocActionPtr
			  (
			    new insight::cad::Distance(measpts_p1_, measpts_p2_)
			  ),
			  true
			);

		    measpts_p1_.reset();
		    measpts_p2_.reset();
		    getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_VERTEX) );
		    cimode_=CIM_Normal;
		  }
	      }



            }
          else if (cimode_==CIM_InsertPointIDs)
            {
              myContext_->InitSelected();
              if (myContext_->MoreSelected())
                {
                  TopoDS_Shape v = myContext_->SelectedShape();
                  TopoDS_Vertex vv = TopoDS::Vertex(v);
                  gp_Pnt vp = BRep_Tool::Pnt(vv);
                  if (!selpts_)
                    {
                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
                        {
                          // restrict further selection to current shape
                          getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_VERTEX) );
                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_VERTEX) );

                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Vertex));

                          insight::cad::FeatureID vid = parent->solidmodel().vertexID(v);
                          selpts_->add(vid);
                          emit sendStatus(boost::str(boost::format("Selected vertex %d. Select next vertex, end with right click.")%vid).c_str());
                        }
                    }
                  else
                    {
                      insight::cad::FeatureID vid = selpts_->model()->vertexID(v);
                      selpts_->add(vid);
                      emit sendStatus(boost::str(boost::format("Selected vertex %d. Select next vertex, end with right click.")%vid).c_str());
                    }
                }
            }
          else if (cimode_==CIM_InsertEdgeIDs)
            {
              myContext_->InitSelected();
              if (myContext_->MoreSelected())
                {
                  TopoDS_Shape e = myContext_->SelectedShape();
                  if (!selpts_)
                    {
                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
                        {
                          // restrict further selection to current shape
                          getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_EDGE) );
                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_EDGE) );

                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Edge));

                          insight::cad::FeatureID eid = parent->solidmodel().edgeID(e);
                          selpts_->add(eid);
                          emit sendStatus(boost::str(boost::format("Selected edge %d. Select next edge, end with right click.")%eid).c_str());
                        }
                    }
                  else
                    {
                      insight::cad::FeatureID eid = selpts_->model()->edgeID(e);
                      selpts_->add(eid);
                      emit sendStatus(boost::str(boost::format("Selected edge %d. Select next edge, end with right click.")%eid).c_str());
                    }
                }
            }
          else if (cimode_==CIM_InsertFaceIDs)
            {
              myContext_->InitSelected();
              if (myContext_->MoreSelected())
                {
                  TopoDS_Shape f = myContext_->SelectedShape();
                  TopoDS_Face ff = TopoDS::Face(f);
                  if (!selpts_)
                    {
                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
                        {
                          // restrict further selection to current shape
                          getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_FACE) );
                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_FACE) );

                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Face));

                          insight::cad::FeatureID fid = parent->solidmodel().faceID(f);
                          selpts_->add(fid);
                          emit sendStatus(boost::str(boost::format("Selected face %d. Select next face, end with right click.")%fid).c_str());
                        }
                    }
                  else
                    {
                      insight::cad::FeatureID fid = selpts_->model()->faceID(f);
                      selpts_->add(fid);
                      emit sendStatus(boost::str(boost::format("Selected face %d. Select next face, end with right click.")%fid).c_str());
                    }
                }
            }

	  break;

	case CurAction3d_Picking:
	  // Shouldn't get here yet
	  if ( myCurrentPoint == myStartPoint )
	    {
	      inputEvent( nFlags & MULTISELECTIONKEY );
	    }
	  else
	    {
	      dragEvent( myStartPoint,
			 myCurrentPoint,
			 nFlags & MULTISELECTIONKEY );
	    }
	  break;

	case CurAction3d_DynamicZooming:
	  viewPrecision( true );
	  break;

	case CurAction3d_WindowZooming:
	  if ( (abs( myCurrentPoint.x() - myStartPoint.x() ) > ValZWMin ) ||
	       (abs( myCurrentPoint.y() - myStartPoint.y() ) > ValZWMin ) )
	    {
	      myView->WindowFitAll( myStartPoint.x(),
				    myStartPoint.y(),
				    myCurrentPoint.x(),
				    myCurrentPoint.y() );
	    }
	  viewPrecision( true );
	  break;

	case CurAction3d_DynamicPanning:
	  break;

	case CurAction3d_GlobalPanning :
	  myView->Place( myCurrentPoint.x(), myCurrentPoint.y(), myCurZoom );
	  break;

	case CurAction3d_DynamicRotation:
	  break;

	default:
	  Standard_Failure::Raise(" Incompatible Current Mode ");
	  break;
	}
    }
}
/*!
  \brief	Middle button up event handler.
  This marks the end of the gesture.
*/
void QoccViewWidget::onMiddleButtonUp(  Qt::KeyboardModifiers /* nFlags */, const QPoint /* point */ )
{
 // setMode( CurAction3d_Nothing );
}

/*!
  \brief	Right button up event handler.
  This marks the end of the gesture.
*/
void QoccViewWidget::onRightButtonUp(  Qt::KeyboardModifiers, const QPoint point )
{
  myCurrentPoint = point;
  /*
  if ( nFlags & CASCADESHORTCUTKEY )
    {
      setMode( CurAction3d_Nothing );
    }
    else*/
    {
      if ( myMode == CurAction3d_Nothing )
	{
	  if (cimode_==CIM_Normal)
	    {
	      displayContextMenu(point);
	    }
	  else if (cimode_==CIM_InsertPointIDs)
	    {
	      QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?vid=(";
	      int j=0;
	      for (insight::cad::FeatureID i: selpts_->data())
		{
		  text+=QString::number( i );
		  if (j++ < selpts_->size()-1) text+=",";
		}
	      text+=")\n";
	      emit insertNotebookText(text);

	      getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_VERTEX) );
	      cimode_=CIM_Normal;
	    }
	  else if (cimode_==CIM_InsertEdgeIDs)
	    {
	      QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?eid=(";
	      int j=0;
	      for (insight::cad::FeatureID i: selpts_->data())
		{
		  text+=QString::number( i );
		  if (j++ < selpts_->size()-1) text+=",";
		}
	      text+=")\n";
	      emit insertNotebookText(text);

	      getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_EDGE) );
	      cimode_=CIM_Normal;
	    }
	  else if (cimode_==CIM_InsertFaceIDs)
	    {
	      QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?fid=(";
	      int j=0;
	      for (insight::cad::FeatureID i: selpts_->data())
		{
		  text+=QString::number( i );
		  if (j++ < selpts_->size()-1) text+=",";
		}
	      text+=")\n";
	      emit insertNotebookText(text);

	      getContext()->Deactivate( AIS_Shape::SelectionMode(TopAbs_FACE) );
	      cimode_=CIM_Normal;
	    }
	  //	  emit popupMenu ( this, point );
        }
      else
	{
	  setMode( CurAction3d_Nothing );
	}
    }
}

/*!
  \brief	Mouse move event handler.
  \param	buttons
  \param	nFlags
  \param	point
  \return Nothing
*/
void QoccViewWidget::onMouseMove( Qt::MouseButtons buttons,
				  Qt::KeyboardModifiers nFlags,
				  const QPoint point,
     Qt::KeyboardModifiers curFlags )
{
  myCurrentPoint = point;

// cout<< ( curFlags & ZOOMSHORTCUTKEY )<<" "<< ( curFlags & PANSHORTCUTKEY )<<" "<<( curFlags & ROTATESHORTCUTKEY ) <<curFlags<<myMode<<endl;
  
//   if ( buttons & Qt::LeftButton  ||
//        buttons & Qt::RightButton ||
//        buttons & Qt::MidButton )
    {
      switch ( myMode )
	{
	case CurAction3d_Nothing:
	  if ( curFlags & ZOOMSHORTCUTKEY )
	  {
	    myStartPoint = point;
	    setMode(CurAction3d_DynamicZooming);
	  }  
	  else if ( curFlags & PANSHORTCUTKEY )
	  {
	    myStartPoint = point;
	    setMode(CurAction3d_DynamicPanning);
	  }  
	  else if ( curFlags & ROTATESHORTCUTKEY )
	  {
	    myStartPoint = point;
	    setMode(CurAction3d_DynamicRotation);
	    myView->StartRotation( point.x(), point.y() );
	  }
	  break;

	case CurAction3d_Picking:
	  if ( buttons & Qt::LeftButton)
	  {
	  // Shouldn't get here yet
	  drawRubberBand ( myStartPoint, myCurrentPoint );
	  dragEvent( myStartPoint, myCurrentPoint, nFlags & MULTISELECTIONKEY );
	  break;
	  }

	case CurAction3d_DynamicZooming:
	  myView->Zoom(	myStartPoint.x(),
			myStartPoint.y(),
			myCurrentPoint.x(),
			myCurrentPoint.y() );
	  viewPrecision( true );
	  myStartPoint = myCurrentPoint;
	  if ( !(curFlags & ZOOMSHORTCUTKEY) ) setMode(CurAction3d_Nothing);
	  break;

	case CurAction3d_WindowZooming:
	  if ( buttons & Qt::LeftButton)
	  {
	  drawRubberBand ( myStartPoint, myCurrentPoint );
	  }
	  break;

	case CurAction3d_DynamicPanning:
	  myView->Pan( myCurrentPoint.x() - myStartPoint.x(),
		       myStartPoint.y() - myCurrentPoint.y() );
	  myStartPoint = myCurrentPoint;
	  if ( !(curFlags & PANSHORTCUTKEY) ) setMode(CurAction3d_Nothing);
	  break;

	case CurAction3d_GlobalPanning:
	  break;

	case CurAction3d_DynamicRotation:
	  myView->Rotation( myCurrentPoint.x(), myCurrentPoint.y() );
	  if ( !(curFlags & ROTATESHORTCUTKEY) ) setMode(CurAction3d_Nothing);
	  break;

	default:
	  Standard_Failure::Raise( "Incompatible Current Mode" );
	  break;
	}
    }
//   else
    {
      moveEvent( myCurrentPoint );
    }
}
/*!
  \brief	Move event detection handler
*/
AIS_StatusOfDetection QoccViewWidget::moveEvent( QPoint point )
{
  AIS_StatusOfDetection status;
  status = myContext_->MoveTo( point.x(), point.y(), myView
#if (OCC_VERSION_MAJOR>=7)
   , true
#endif
);
  return status;
}

/*!
  \brief	Drag event handler.
  \param	startPoint	The gesture start point.
  \param	endPoint	The gesture end point.
  \param	multi		Allows selection of multiple objects.
  \return The status of pick.
*/
AIS_StatusOfPick QoccViewWidget::dragEvent( const QPoint startPoint, const QPoint endPoint, const bool multi )
{
  AIS_StatusOfPick pick = AIS_SOP_NothingSelected;
  if (multi)
    {
      pick = myContext_->ShiftSelect( std::min (startPoint.x(), endPoint.x()),
				     std::min (startPoint.y(), endPoint.y()),
				     std::max (startPoint.x(), endPoint.x()),
				     std::max (startPoint.y(), endPoint.y()),
				     myView
#if (OCC_VERSION_MAJOR>=7)
                    , true
#endif
      );
    }
  else
    {
      pick = myContext_->Select( std::min (startPoint.x(), endPoint.x()),
				std::min (startPoint.y(), endPoint.y()),
				std::max (startPoint.x(), endPoint.x()),
				std::max (startPoint.y(), endPoint.y()),
				myView
#if (OCC_VERSION_MAJOR>=7)
                    , true
#endif
        );
    }
  emit graphicalSelectionChanged(getSelectedItem(), this);
  return pick;
}
/*!
  \brief	This handles object highlighting during movement of the mouse across the view.
  \param	multi	Selects multiple objects if true (default false).
  \return The status of the objects under the cursor
*/
AIS_StatusOfPick QoccViewWidget::inputEvent( bool multi )
{
  AIS_StatusOfPick pick = AIS_SOP_NothingSelected;

  if (multi)
    {
      pick = myContext_->ShiftSelect(
#if (OCC_VERSION_MAJOR>=7)
                    true
#endif          
            );
    }
  else
    {
      pick = myContext_->Select(
#if (OCC_VERSION_MAJOR>=7)
                    true
#endif                    
            );
    }
  if ( pick != AIS_SOP_NothingSelected )
    {
      emit graphicalSelectionChanged(getSelectedItem(), this);
    }
  return pick;
}

bool QoccViewWidget::dump(Standard_CString theFile)
{
  redraw();
  return myView->Dump(theFile);
}

/*!
  \brief This function sets the current cursor for the given interraction mode.
  \param mode		The interraction mode
*/
void QoccViewWidget::setMode( const CurrentAction3d mode )
{
  if ( mode != myMode )
    {
      switch( mode )
	{
	case CurAction3d_DynamicPanning:
	  setCursor( Qt::SizeAllCursor );
	  break;
	case CurAction3d_DynamicZooming:
	  setCursor( Qt::CrossCursor );
	  break;
	case CurAction3d_DynamicRotation:
	  setCursor( Qt::CrossCursor );
	  break;
	case CurAction3d_GlobalPanning:
	  setCursor( Qt::CrossCursor );
	  break;
	case CurAction3d_WindowZooming:
	  setCursor( Qt::PointingHandCursor );
	  break;
	case CurAction3d_Nothing:
	  //setCursor( myCrossCursor );
	  setCursor( Qt::CrossCursor );
	  break;
	default:
	  setCursor( Qt::ArrowCursor );
	  break;
	}
      myMode = mode;
    }
}

/*!
  \brief This is a Utility function for rounding the input value to a specific DP
*/
Standard_Real QoccViewWidget::precision( Standard_Real aReal )
{
  Standard_Real preciseReal;
  Standard_Real thePrecision = std::max (myPrecision, viewPrecision());
	
  if ( myPrecision != 0.0 )
    {
      preciseReal =  SIGN(aReal) * floor((std::abs(aReal) + thePrecision * 0.5) / thePrecision) * thePrecision;
    }
  else
    {
      preciseReal = aReal;
    }
  return preciseReal;
}

/*! ------------------------------------------------------------------------------------
  \brief	ConvertToPlane convert 2d window position to 3d point on priviledged plane.
  This routine was provided by Matra Datavision during Foundation training.
  There still appears to be a pixel error in y co-ordinate transforms.
  \param	Xs				The screen's x co-ordinate (in)
  \param	Ys				The screen's y co-ordinate (in)
  \param	X				The output x position on the privileged plane (out)
  \param	Y				The output y position on the privileged plane (out)
  \param	Z				The output z position on the privileged plane (out)
  \return	Standard_Boolean indicating success or failure
*/
Standard_Boolean QoccViewWidget::convertToPlane(const Standard_Integer Xs,
						const Standard_Integer Ys,
						Standard_Real& X,
						Standard_Real& Y,
						Standard_Real& Z)
{
  Standard_Real Xv, Yv, Zv;
  Standard_Real Vx, Vy, Vz;
  gp_Pln aPlane(myView->Viewer()->PrivilegedPlane());

#ifdef OCC_PATCHED
  myView->Convert( Xs, Ys, Xv, Yv, Zv ); 
#else
  // The + 1 overcomes a fault in OCC, in "OpenGl_togl_unproject_raster.c",
  // which transforms the Y axis ordinate. The function uses the height of the
  // window, not the Y maximum which is (height - 1).
  myView->Convert( Xs, Ys + 1, Xv, Yv, Zv ); 
#endif 

  myView->Proj( Vx, Vy, Vz );
  gp_Lin aLine(gp_Pnt(Xv, Yv, Zv), gp_Dir(Vx, Vy, Vz));
  IntAna_IntConicQuad theIntersection( aLine, aPlane, Precision::Angular() );
  if (theIntersection.IsDone())
    {
      if (!theIntersection.IsParallel())
	{
	  if (theIntersection.NbPoints() > 0)
	    {
	      gp_Pnt theSolution(theIntersection.Point(1));
	      X = theSolution.X();
	      Y = theSolution.Y();
	      Z = theSolution.Z();
	      return Standard_True;
	    }
	}
    }
  return Standard_False;
}
/*!
  \brief	Draws the rubberband box
  This function is designed to reduce "flicker" as the box is redrawn,
  especially when the origin in the bottom corner of the window
  \param	origin		A QPoint defining the screen origin
  \param	position	A QPoint defining the current cursor screen location
*/
void QoccViewWidget::drawRubberBand( const QPoint origin, const QPoint position )
{
  if ( myRubberBand )
    {
      redraw();
      hideRubberBand();
      myRubberBand->setGeometry( QRect( origin, position ).normalized() );
      showRubberBand();
    }
}
/*!
  \brief	Shows the rubberband box
*/
void QoccViewWidget::showRubberBand( void )
{
  if ( myRubberBand )
    {
      myRubberBand->show();
    }
}
/*!
  \brief	Hides the rubberband box
*/
void QoccViewWidget::hideRubberBand( void )
{
  if ( myRubberBand )
    {
      myRubberBand->hide();
    }
}

#if (OCC_VERSION_MAJOR<7)
/*!
  \brief	Static OpenCascade callback proxy
*/
int QoccViewWidget::paintCallBack (Aspect_Drawable /* drawable */, 
				   void* aPointer,
				   Aspect_GraphicCallbackStruct* /* data */)
{
  QoccViewWidget *aWidget = (QoccViewWidget *) aPointer;
  aWidget->paintOCC();
  return 0;
}

/*!
  \brief	The OpenGL paint routine in the callback.
*/
void QoccViewWidget::paintOCC( void )
{
  glDisable( GL_LIGHTING ); 
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();

  GLfloat left   = -1.0f;
  GLfloat right  =  1.0f;
  GLfloat bottom = -1.0f;
  GLfloat top    =  1.0f;
  GLfloat depth  =  1.0f;

  glOrtho( left, right, bottom, top, 1.0, -1.0 );

#ifndef OCC_PATCHED
  glEnable(GL_BLEND);
  if (myView->ColorScaleIsDisplayed())
    {
      // Not needed on patched OCC 6.2 versions, but is the lowest
      // common denominator working code on collaborators OpenGL
      // graphics cards.
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
    }
#endif

//  // Gradient Background
//   glBegin( GL_QUADS);
//   {
//     glColor4f  (  0.1f, 0.1f, 0.1f, 1.0f );
//     glVertex3d (  left, bottom, depth );
//     glVertex3d ( right, bottom, depth );
//     glColor4f  (  0.8f, 0.8f, 0.9f, 1.0f );
//     glVertex3d ( right,    top, depth );
//     glVertex3d (  left,    top, depth );
//   }
//   glEnd();

  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

}
#endif

/*!
  \brief	This routine calculates the minimum sensible precision for the point 
  selection routines, by setting an minumum resolution to a decade one
  higher than the equivalent pixel width.
  \param	resized		Indicates that recaculation os required due to state
  changes in the view.
*/
Standard_Real QoccViewWidget::viewPrecision( bool resized )
{

  Standard_Real X1, Y1, Z1;
  Standard_Real X2, Y2, Z2;

  if (resized || myViewPrecision == 0.0)
    {
      myView->Convert( 0, 0, X1, Y1, Z1 ); 
      myView->Convert( 1, 0, X2, Y2, Z2 ); 
      Standard_Real pixWidth = X2 - X1;
      if ( pixWidth != 0.0 )
	{
	  // Return the precision as the next highest decade above the pixel width
	  myViewPrecision = std::pow (10.0, std::floor(std::log10( pixWidth ) + 1.0));
	}
      else
	{
	  // Return the user precision if window not defined
	  myViewPrecision = myPrecision; 
	}
    }
  return myViewPrecision;
}
//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------

OCCViewScreenshots::OCCViewScreenshots(Handle_AIS_InteractiveContext& context, QString initPath)
: QDialog(NULL)
{
  format_ = "pnm";
  initialPath_ = initPath;

  QVBoxLayout *l = new QVBoxLayout(this);
  //setModal(false);
  resize(1000,500);

  occWidget_ = new QoccViewWidget(this, context);
  occWidget_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  l->addWidget(occWidget_);
  QFrame* frame = new QFrame(this);
  frame->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
  l->addWidget(frame);

  QHBoxLayout *h = new QHBoxLayout(frame);

  QPushButton *closeBtn = new QPushButton("close");
  h->addWidget(closeBtn);
  QPushButton *okBtn = new QPushButton("screen shot");
  h->addWidget(okBtn);

  connect(okBtn, &QPushButton::clicked, this, &OCCViewScreenshots::screenShot);
  connect(closeBtn, &QPushButton::clicked, this, &OCCViewScreenshots::accept);
}

void OCCViewScreenshots::screenShot()
{
  QFileInfo fileName;
  fileName.setFile( QFileDialog::getSaveFileName(NULL, QObject::tr("Save As"),
                            initialPath_,
                            QObject::tr("%1 Files (*.%2);;All Files (*)")
                            .arg(format_.toUpper())
                            .arg(format_)) );

  files.append(fileName.absoluteFilePath());
  cout << "Exporting screenshot to: " << fileName.absoluteFilePath().toStdString() << endl;
  char fName[fileName.absoluteFilePath().length()+10];
  strcpy(fName, fileName.absoluteFilePath().toStdString().c_str());
  const Handle_V3d_View& myView = occWidget_->getView();
  myView->Dump(fName);
}

