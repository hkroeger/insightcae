#include <cmath>
#include <iostream>

#include <GL/gl.h>
#include <GL/glu.h>

#include <QtGui>

#include "qoccviewwidget.h"
#include "qoccinternal.h"

#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_PositionalLight.hxx>


QoccViewWidget::QoccViewWidget
( 
 const Handle_AIS_InteractiveContext& aContext, 
 QWidget *parent, 
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
    myButtonFlags	( Qt::NoButton )
{
  myContext = aContext;
  // Needed to generate mouse events
  setMouseTracking( true );
  
  // Avoid Qt background clears to improve resizing speed,
  // along with a couple of other attributes
  setAutoFillBackground( false );				
  setAttribute( Qt::WA_NoSystemBackground );	
  
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
  if (myRubberBand)
    {
      // If you don't set a style, QRubberBand doesn't work properly
      // take this line out if you don't believe me.
      myRubberBand->setStyle( (QStyle*) new QPlastiqueStyle() );
    }
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
void QoccViewWidget::initializeOCC(const Handle_AIS_InteractiveContext& aContext)
{
  Aspect_RenderingContext rc = 0;
  myContext = aContext;
  myViewer  = myContext->CurrentViewer();
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
     Handle_Graphic3d_GraphicDevice::DownCast( myContext->CurrentViewer()->Device() ),
     (int) hi, (int) lo, Xw_WQ_SAMEQUALITY, Quantity_NOC_BLACK 

    );
#endif // WNT
	
  if (!myView.IsNull())
    {
      // Set my window (Hwnd) into the OCC view
      myView->SetWindow( myWindow, rc , paintCallBack, this  );
      // Set up axes (Trihedron) in lower left corner.
      myView->SetScale( 2 );			// Choose a "nicer" intial scale
      
      // Set up axes (Trihedron) in lower left corner.
#ifdef OCC_PATCHED
      myView->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.1, V3d_ZBUFFER );
#else
      myView->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.1, V3d_WIREFRAME );
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

      myView->EnableGLLight(false);
      myViewer->InitActiveLights();
      while ( myViewer->MoreActiveLights ()) {
        Handle_V3d_Light myLight = myViewer->ActiveLight();
        myViewer->SetLightOff(myLight);
        myViewer->NextActiveLights();
      }
      Handle_V3d_Light myLight = new V3d_AmbientLight(myViewer,Quantity_NOC_WHITE);
      myView->SetLightOn(myLight);
      myView->SetLightOn(new V3d_PositionalLight (myViewer,  10000,-3000,30000,  Quantity_NOC_ANTIQUEWHITE3, 1., 0.));
      myView->SetLightOn(new V3d_PositionalLight (myViewer,  10000,-3000,-30000,  Quantity_NOC_ANTIQUEWHITE3, 1., 0.));
      myView->SetLightOn(new V3d_PositionalLight (myViewer,-30000,-3000,-10000,  Quantity_NOC_ANTIQUEWHITE3, 1., 0.));
      myView->UpdateLights();

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
	  initializeOCC( myContext );
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
  if( myContext->CurrentViewer()->Grid()->IsActive() && myGridSnap )
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
  std::cout<<e->modifiers()<<std::endl;
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
  std::cout<<e->modifiers()<<std::endl;
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
      QApplication::setOverrideCursor( Qt::WaitCursor );
      myView->SetComputedMode( Standard_False );
      QApplication::restoreOverrideCursor();
    }
}

void QoccViewWidget::hiddenLineOn()
{
  if(!myView.IsNull())
    {
      QApplication::setOverrideCursor( Qt::WaitCursor );
      myView->SetComputedMode( Standard_True );
      QApplication::restoreOverrideCursor();
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
      myView->SetBackgroundColor(Quantity_TOC_RGB,R1,G1,B1);
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
    {
      emit popupMenu ( this, point );
    }
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
  emit selectionChanged();
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
	  emit popupMenu ( this, point );
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
  status = myContext->MoveTo( point.x(), point.y(), myView );
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
      pick = myContext->ShiftSelect( std::min (startPoint.x(), endPoint.x()),
				     std::min (startPoint.y(), endPoint.y()),
				     std::max (startPoint.x(), endPoint.x()),
				     std::max (startPoint.y(), endPoint.y()),
				     myView );
    }
  else
    {
      pick = myContext->Select( std::min (startPoint.x(), endPoint.x()),
				std::min (startPoint.y(), endPoint.y()),
				std::max (startPoint.x(), endPoint.x()),
				std::max (startPoint.y(), endPoint.y()),
				myView );
    }
  emit selectionChanged();
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
      pick = myContext->ShiftSelect();
    }
  else
    {
      pick = myContext->Select();
    }
  if ( pick != AIS_SOP_NothingSelected )
    {
      emit selectionChanged();
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

  glBegin( GL_QUADS);
  {
    glColor4f  (  0.1f, 0.1f, 0.1f, 1.0f );
    glVertex3d (  left, bottom, depth );
    glVertex3d ( right, bottom, depth );
    glColor4f  (  0.8f, 0.8f, 0.9f, 1.0f );
    glVertex3d ( right,    top, depth );
    glVertex3d (  left,    top, depth );
  }
  glEnd();

  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

}
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
  resize(1200,900);

  occWidget_ = new QoccViewWidget(context,this);
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

  connect(okBtn,SIGNAL(clicked()),this,SLOT(screenShot()),Qt::DirectConnection);
  connect(closeBtn,SIGNAL(clicked()),this,SLOT(accept()),Qt::DirectConnection);
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
  strcpy(fName, fileName.absoluteFilePath().toAscii().data());
  const Handle_V3d_View& myView = occWidget_->getView();
  myView->Dump(fName);
}

