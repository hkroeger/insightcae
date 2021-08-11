#if !defined _WIN32
#define QT_CLEAN_NAMESPACE         /* avoid definition of INT32 and INT8 */
#endif


#include <QApplication>
#include <QPainter>
#include <QMenu>
#include <QColorDialog>
#include <QCursor>
#include <QFileInfo>
#include <QFileDialog>
#include <QMouseEvent>
#include <QRubberBand>
#include <QMdiSubWindow>
#include <QStyleFactory>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QBitmap>
#include <QDebug>

#include <Graphic3d_GraphicDriver.hxx>
#include <Graphic3d_TextureEnv.hxx>
#include <OpenGl_GraphicDriver.hxx>

#include <Aspect_DisplayConnection.hxx>
#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_NameOfMaterial.hxx>
#if !defined(_WIN32) && !defined(__WIN32__) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX))
#include <OSD_Environment.hxx>
#endif
#include <TCollection_AsciiString.hxx>
#include <Aspect_DisplayConnection.hxx>


#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <V3d_PositionalLight.hxx>
#include <Aspect_Grid.hxx>
#include "Graphic3d_AspectFillArea3d.hxx"
#include "AIS_Plane.hxx"
#include "AIS_Point.hxx"
#include "IntAna_IntConicQuad.hxx"
#include "AIS_ListOfInteractive.hxx"
#include "AIS_ListIteratorOfListOfInteractive.hxx"

#include <cmath>
#include <iostream>

#include "base/exception.h"
#include "occtwindow.h"
#include "occtools.h"
#include "pointertransient.h"
#include "cadtypes.h"
#include "cadpostprocactions.h"
#include "qmodeltree.h"
#include "qdatumitem.h"
#include "datum.h"
#include "pointertransient.h"
#include "qmodelstepitem.h"

#include "qoccviewwidget.h"

#include "viewwidgetinsertids.h"


using namespace std;

static Handle(OpenGl_GraphicDriver) aGraphicDriver;


void QoccViewWidget::addLights()
{
  Handle_V3d_AmbientLight L1 = new V3d_AmbientLight(
#if OCC_VERSION_MAJOR<7
        myViewer,
#endif
        Quantity_NOC_WHITE );

#if !(OCC_VERSION_MAJOR<7)
  L1->SetIntensity(0.4);
  myViewer->AddLight(L1);
#endif

  lights_.push_back(L1);

  Handle_V3d_DirectionalLight L2 = new V3d_DirectionalLight(
#if OCC_VERSION_MAJOR<7
        myViewer,
#endif
        V3d_XnegYnegZneg, Quantity_NOC_WHITE, true );

#if !(OCC_VERSION_MAJOR<7)
  L2->SetIntensity(0.8);
  myViewer->AddLight(L2);
#endif

  lights_.push_back(L2);

  myViewer->SetLightOn();

}



QoccViewWidget::QoccViewWidget(QWidget *parent)
  : QWidget(parent),
    currentNavigationAction_(nullptr),
    navigationManager_(
      std::make_shared<TouchpadNavigationManager>(currentNavigationAction_, this, myView) ),
    myGridSnap          ( Standard_False ),
    myDetection         ( AIS_SOD_Nothing ),
    myPrecision		( 0.0001 ),
    myViewPrecision     ( 0.0 ),
    myKeyboardFlags     ( Qt::NoModifier ),
    myButtonFlags	( Qt::NoButton ),
    showGrid            ( false )/*,
    cimode_             ( CIM_Normal )*/
{
  insight::CurrentExceptionContext ex("construct QoccViewWidget");

  // Needed to generate mouse events
  setMouseTracking( true );
  setBackgroundRole( QPalette::NoRole );
  setAttribute( Qt::WA_NoSystemBackground );
  setAttribute( Qt::WA_PaintOnScreen );
  setFocusPolicy(Qt::StrongFocus);

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
//  myRubberBand = new QRubberBand( QRubberBand::Rectangle, this );

  if (aGraphicDriver.IsNull())
  {
    Handle(Aspect_DisplayConnection) aDisplayConnection;
#if !defined(_WIN32) && !defined(__WIN32__) && (!defined(__APPLE__) || defined(MACOSX_USE_GLX))
    aDisplayConnection = new Aspect_DisplayConnection(getenv("DISPLAY"));
#endif
    aGraphicDriver = new OpenGl_GraphicDriver (aDisplayConnection);
  }

  myViewer = new V3d_Viewer (
        aGraphicDriver
#if OCC_VERSION_MAJOR<7
        ,
       TCollection_ExtendedString("Visu3D").ToExtString(),
       "",
       1000,
       V3d_XposYnegZpos,
       Quantity_NOC_GRAY30,
       V3d_ZBUFFER,
       V3d_GOURAUD,
       V3d_WAIT,
       Standard_True,
       Standard_True,
       V3d_TEX_NONE
#endif
        );
#if OCC_VERSION_MAJOR>=7
  myViewer->SetDefaultViewSize (1000);
  myViewer->SetDefaultViewProj (V3d_XposYnegZpos);
  myViewer->SetComputedMode (Standard_True);
  myViewer->SetDefaultComputedMode (Standard_True);
#endif

  addLights();

#if !(OCC_VERSION_MAJOR<7)
  myViewer->SetDefaultShadingModel(Graphic3d_TOSM_FRAGMENT);
#endif

  myContext_ = new AIS_InteractiveContext (myViewer);

  myView = myContext_->CurrentViewer()->CreateView();

//  auto *w = new QWindow( this->windowHandle() );
//  hWnd = new Xw_Window
//      (
//        myContext_->CurrentViewer()->Driver()->GetDisplayConnection(),
//        winId()
//      );
  hWnd = new OcctWindow(this);

  myView->SetWindow( hWnd );
  if ( !hWnd->IsMapped() )
  {
    hWnd->Map();
  }


  myView->SetBackgroundColor (Quantity_NOC_WHITE);
  myView->MustBeResized();


  // Set up axes (Trihedron) in lower left corner.
  myView->SetScale( 2 );			// Choose a "nicer" intial scale

  // Set up axes (Trihedron) in lower left corner.
  myView->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_BLACK, 0.1, V3d_ZBUFFER );
//  myView->SetShadingModel(V3d_PHONG);

  myView->SetProj( V3d_Zpos );
  myView->SetUp( V3d_Xpos );

//  myMode = CurAction3d_Nothing;

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
// if ( myRubberBand )
//    {
//      delete myRubberBand;
//    }
}


/*!
  \brief	Returns a NULL QPaintEngine
  This should result in a minor performance benefit.
*/
QPaintEngine* QoccViewWidget::paintEngine() const
{
  return nullptr;
}



/*!
\brief	Paint Event
		Called when the Widget needs to repaint itself
\param	e The event data.
*/
void QoccViewWidget::paintEvent ( QPaintEvent * /* e */)
{
  insight::dbg()<<"paint event"<<std::endl;

  if (!myView.IsNull())
  {
    try
    {
#if OCC_VERSION_MAJOR<7
      myView->Redraw();
#else
      myView->InvalidateImmediate();
      FlushViewEvents (myContext_, myView, true);
#endif
    }
    catch (Standard_Failure e)
    {
      insight::Warning(
            e.GetMessageString() ? e.GetMessageString() : "Error during redraw (no error message)" );
    }
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
  insight::dbg()<<"resize event"<<std::endl;
//  myViewResized = Standard_True;

  //  QApplication::syncX();
    if( !myView.IsNull() )
    {
      myView->MustBeResized();
    }
}





void QoccViewWidget::mousePressEvent( QMouseEvent* e )
{
  insight::dbg()<<"mousepressevent"<<std::endl;
  if (!myView.IsNull())
  {
    if ( e->button() & Qt::LeftButton )
      {
        navigationManager_->onLeftButtonDown( e->modifiers(), e->pos() );
        if (currentNavigationAction_)
          currentNavigationAction_->onLeftButtonDown( e->modifiers(), e->pos() );
        if (currentUserActivity_)
          currentUserActivity_->onLeftButtonDown( e->modifiers(), e->pos() );
      }
    else if ( e->button() & Qt::RightButton )
      {
        navigationManager_->onRightButtonDown( e->modifiers(), e->pos() );
        if (currentNavigationAction_)
          currentNavigationAction_->onRightButtonDown( e->modifiers(), e->pos() );
        if (currentUserActivity_)
          currentUserActivity_->onRightButtonDown( e->modifiers(), e->pos() );
      }
    else if ( e->button() & Qt::MidButton )
      {
        navigationManager_->onMiddleButtonDown( e->modifiers(), e->pos() );
        if (currentNavigationAction_)
          currentNavigationAction_->onMiddleButtonDown( e->modifiers(), e->pos() );
        if (currentUserActivity_)
          currentUserActivity_->onMiddleButtonDown( e->modifiers(), e->pos() );
      }

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();
  }
  else
    e->ignore();
}





void QoccViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  insight::dbg()<<"mousereleaseevent"<<std::endl;
  if (!myView.IsNull())
  {
    if ( e->button() & Qt::LeftButton )
      {
        navigationManager_->onLeftButtonUp( e->modifiers(), e->pos() );
        if (currentNavigationAction_)
          currentNavigationAction_->onLeftButtonUp( e->modifiers(), e->pos() );
        if (currentUserActivity_)
          currentUserActivity_->onLeftButtonUp( e->modifiers(), e->pos() );
      }
    else if ( e->button() & Qt::RightButton )
      {
        navigationManager_->onRightButtonUp( e->modifiers(), e->pos() );
        if (currentNavigationAction_)
          currentNavigationAction_->onRightButtonUp( e->modifiers(), e->pos() );
        else if (currentUserActivity_)
          currentUserActivity_->onRightButtonUp( e->modifiers(), e->pos() );
        else
        {
          displayContextMenu(e->pos());
        }
      }
    else if ( e->button() & Qt::MidButton )
      {
        navigationManager_->onMiddleButtonUp( e->modifiers(), e->pos() );
        if (currentNavigationAction_)
          currentNavigationAction_->onMiddleButtonUp( e->modifiers(), e->pos() );
        if (currentUserActivity_)
          currentUserActivity_->onMiddleButtonUp( e->modifiers(), e->pos() );
      }

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();
  }
  else
    e->ignore();
}




/*!
\brief	Mouse move event, driven from application message loop
\param	e The event data.
*/
void QoccViewWidget::mouseMoveEvent(QMouseEvent* e)
{
//  insight::dbg()<<"mousemoveevent"<<std::endl;
  if (!myView.IsNull())
  {

//    Standard_Real X, Y, Z;
  
//    myCurrentPoint = e->pos();
//    //Check if the grid is active and that we're snapping to it
//    if( myContext_->CurrentViewer()->Grid()->IsActive() && myGridSnap )
//      {
//        myView->ConvertToGrid
//          (
//           myCurrentPoint.x(),
//           myCurrentPoint.y(),
//           myV3dX,
//           myV3dY,
//           myV3dZ
//          );
//        emit mouseMoved( myV3dX, myV3dY, myV3dZ );
//      }
//    else //	this is the standard case
//      {
//        if (convertToPlane
//            (
//             myCurrentPoint.x(),
//             myCurrentPoint.y(),
//             X, Y, Z
//            ) )
//          {
//            myV3dX = precision( X );
//            myV3dY = precision( Y );
//            myV3dZ = precision( Z );
//            emit mouseMoved( myV3dX, myV3dY, myV3dZ );
//          }
//        else
//          {
//            emit sendStatus ( tr("Indeterminate Point") );
//          }
//      }

    try // sometimes fails in WIN32
    {
      myContext_->MoveTo(
          e->pos().x(), e->pos().y(),
          myView
#if (OCC_VERSION_MAJOR>=7)
          , true
#endif
      );
    }
    catch (...)
    {
      insight::dbg()<<"Failed moveTo"<<std::endl;
    }

    navigationManager_->onMouseMove( e->buttons(), myKeyboardFlags, e->pos(), e->modifiers() );
    if (currentNavigationAction_)
      currentNavigationAction_->onMouseMove( e->buttons(), myKeyboardFlags, e->pos(), e->modifiers() );
    if (currentUserActivity_)
      currentUserActivity_->onMouseMove( e->buttons(), myKeyboardFlags, e->pos(), e->modifiers() );

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();

  }
  else
    e->ignore();

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
//  myButtonFlags = Qt::NoButton;
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
  insight::dbg()<<"wheelevent"<<std::endl;
  if (!myView.IsNull())
    {
//      Standard_Real currentScale = myView->Scale();
//      if (e->angleDelta().y() > 0)
//	{
//	  currentScale *= 1.10; // +10%
//	}
//      else
//	{
//	  currentScale /= 1.10; // -10%
//	}
//      myView->SetScale( currentScale );
      navigationManager_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());
      if (currentNavigationAction_)
        currentNavigationAction_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());
      if (currentUserActivity_)
        currentUserActivity_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());

      if (currentUserActivity_ && currentUserActivity_->finished())
        currentUserActivity_.reset();

    }
  else
    {
      e->ignore();
    }
}




void QoccViewWidget::keyPressEvent(QKeyEvent* e)
{
  insight::dbg()<<"key press event"<<std::endl;
////   std::cout<<e->modifiers()<<std::endl;
//    if ( ( e->modifiers() & ZOOMSHORTCUTKEY ) && (myMode == CurAction3d_Nothing) )
//    {
//      setMode(CurAction3d_DynamicZooming);
//    }
//    else if ( ( e->modifiers() & PANSHORTCUTKEY ) && (myMode == CurAction3d_Nothing) )
//    {
//      setMode(CurAction3d_DynamicPanning);
//    }
//    else if ( ( e->modifiers() & ROTATESHORTCUTKEY ) && (myMode == CurAction3d_Nothing) )
//    {
//      setMode(CurAction3d_DynamicRotation);
//    }
//    else
//      QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Escape)
  {
    currentNavigationAction_.reset();
    currentUserActivity_.reset();
  }


  navigationManager_->onKeyPress(e->modifiers(), e->key());

  if (currentNavigationAction_)
    currentNavigationAction_->onKeyPress(e->modifiers(), e->key());

  if (currentUserActivity_)
    currentUserActivity_->onKeyPress(e->modifiers(), e->key());

  if (currentUserActivity_ && currentUserActivity_->finished())
    currentUserActivity_.reset();

  QWidget::keyPressEvent(e);
}




void QoccViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  insight::dbg()<<"key release event"<<std::endl;
////   std::cout<<e->modifiers()<<std::endl;
//    if ( !( e->modifiers() & ZOOMSHORTCUTKEY ) && (myMode == CurAction3d_DynamicZooming) )
//    {
//      setMode(CurAction3d_Nothing);
//    }
//    else if ( !( e->modifiers() & PANSHORTCUTKEY ) && (myMode == CurAction3d_DynamicPanning) )
//    {
//      setMode(CurAction3d_Nothing);
//    }
//    else if ( !( e->modifiers() & ROTATESHORTCUTKEY ) && (myMode == CurAction3d_DynamicRotation) )
//    {
//      setMode(CurAction3d_Nothing);
//    }
//    else
//      QWidget::keyReleaseEvent(e);
  navigationManager_->onKeyRelease(e->modifiers(), e->key());

  if (currentNavigationAction_)
    currentNavigationAction_->onKeyRelease(e->modifiers(), e->key());

  if (currentUserActivity_)
    currentUserActivity_->onKeyRelease(e->modifiers(), e->key());

  if (currentUserActivity_ && currentUserActivity_->finished())
    currentUserActivity_.reset();

  QWidget::keyReleaseEvent(e);
}




void QoccViewWidget::onGraphicalSelectionChanged(QDisplayableModelTreeItem* selection, QoccViewWidget* /*viewer*/)
{
  insight::dbg()<<"onGraphicalSelectionChanged"<<std::endl;
    // Remove previously displayed sub objects from display
    for (Handle_AIS_InteractiveObject& o: additionalDisplayObjectsForSelection_)
    {
        getContext()->Erase(o, false);
    }
    additionalDisplayObjectsForSelection_.clear();

    // Display sub objects for current selection
    if (QFeatureItem* ms = dynamic_cast<QFeatureItem*>(selection))
    {
        insight::cad::Feature& sm = ms->solidmodel();

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


///*!
//  \brief	Go idle
//  This called from various locations, and also exposed as a slot.
//*/
//void QoccViewWidget::idle( )
//{
//  setMode( CurAction3d_Nothing );
//}



///*!
//  \brief	The main redraw function
//  This called from various locations.
//*/
//void QoccViewWidget::redraw( bool isPainting )
//{
//  if ( !myView.IsNull() )					// Defensive test.
//    {
//      if ( myViewResized )
//	{
//	  myView->MustBeResized();
//	  viewPrecision( true );
//	}
//      else
//	{
//	  // Don't repaint if we are already redrawing
//	  // elsewhere due to a keypress or mouse gesture
//	  if ( !isPainting || ( isPainting && myButtonFlags == Qt::NoButton ) )
//	    {
//	      myView->Redraw();
//	    }
//	}
//    }
//  myViewResized = Standard_False;
//}


QDisplayableModelTreeItem* QoccViewWidget::getOwnerItem(Handle_AIS_InteractiveObject selected)
{
  insight::CurrentExceptionContext ec("retrieving owner item of interactive shape");

  Handle_Standard_Transient own=selected->GetOwner();
  if (!own.IsNull())
  {
    insight::dbg()<<"owner exists"<<std::endl;

    if (PointerTransient *smo=dynamic_cast<PointerTransient*>(own
#if (OCC_VERSION_MAJOR<7)
              .Access()
#else
              .get()
#endif
      ))
      {
        insight::dbg()<<"owner is PointerTransient"<<std::endl;
        if (QDisplayableModelTreeItem* mi=dynamic_cast<QDisplayableModelTreeItem*>(smo->getPointer()))
        {
          insight::dbg()<<"owner is QDisplayableModelTreeItem"<<std::endl;
          return mi;
        }
      }
  }

  return NULL;
}

QDisplayableModelTreeItem* QoccViewWidget::getSelectedItem()
{
  insight::CurrentExceptionContext ec("looking up the selected CAD feature");

  if (myContext_->HasDetected())
  {
      if (myContext_->DetectedInteractive()->HasOwner())
      {
        insight::dbg()<<"has owner"<<std::endl;
          return getOwnerItem(myContext_->DetectedInteractive());
      }
  }

  return NULL;
}


void QoccViewWidget::connectModelTree(QModelTree* mt) const
{
  connect(mt, &QModelTree::showItem,
          this, &QoccViewWidget::onShow);
  connect(mt, &QModelTree::hideItem,
          this, &QoccViewWidget::onHide);
  connect(mt, &QModelTree::setDisplayMode,
          this, &QoccViewWidget::onSetDisplayMode);
  connect(mt, &QModelTree::setColor,
          this, &QoccViewWidget::onSetColor);
  connect(mt, &QModelTree::setItemResolution,
          this, &QoccViewWidget::onSetResolution);
}

QSize QoccViewWidget::sizeHint() const
{
  QSize s;
  s.setWidth(640);
  s.setHeight(480);
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
      myView->FitAll();
      myView->ZFitAll();
      viewPrecision( true );
    }
}

////-----------------------------------------------------------------------------
///*!
//\brief	Sets up the view for a rubberband window zoom
//\return	Nothing
//*/
//void QoccViewWidget::fitArea( void )
//{
//  setMode( CurAction3d_WindowZooming );
//}

///*!
//\brief	Sets up the view for a dynamic zoom
//\return	Nothing
//*/
//void QoccViewWidget::zoom( void )
//{
//  setMode( CurAction3d_DynamicZooming );
//}

///*!
//\brief	Sets up the view for panning
//\return	Nothing
//*/
//void QoccViewWidget::pan( void )
//{
//  setMode( CurAction3d_DynamicPanning );
//}

///*!
//\brief	Sets up the view for dynamic rotation
//\return	Nothing
//*/
//void QoccViewWidget::rotation( void )
//{
//  setMode( CurAction3d_DynamicRotation );
//}

///*!
//\brief	Sets up the view for global panning, whatever that is!
//\return	Nothing
//*/
//void QoccViewWidget::globalPan()
//{
//  if (!myView.IsNull())
//    {
//      // save the current zoom value
//      myCurZoom = myView->Scale();
//      // Do a Global Zoom
//      myView->FitAll();
//      viewPrecision( true );
//      // Set the mode
//      setMode( CurAction3d_GlobalPanning );
//    }
//}

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
  myView->Redraw(); //redraw();
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
//  Aspect_GridType		myGridType= Aspect_GT_Rectangular;
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
    clipPlane_ = new Graphic3d_ClipPlane(pl);
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


void QoccViewWidget::displayMessage(const QString& /*msg*/)
{
}


Bnd_Box QoccViewWidget::sceneBoundingBox() const
{
  insight::CurrentExceptionContext ec("computing scene bounding box");
    AIS_ListOfInteractive loi;
    myContext_->DisplayedObjects(loi);

    Bnd_Box bbb;
#if OCC_VERSION_MAJOR<7
    for (AIS_ListIteratorOfListOfInteractive i(loi); i.More(); i.Next())
      {
          Handle_AIS_InteractiveObject o = i.Value();
#else
    for (auto i=loi.cbegin(); i!=loi.cend(); i++)
      {
          Handle_AIS_InteractiveObject o = *i;
#endif
          insight::dbg()<<"next item"<<std::endl;
          if (QFeatureItem* it
                = dynamic_cast<QFeatureItem*>(const_cast<QoccViewWidget*>(this)->getOwnerItem(o)))
          {
            arma::mat bb =it->solidmodel().modelBndBox();
//            qDebug()
//                << bb(0,0) << " " << bb(1,0) << " " << bb(2,0) << " "
//                << bb(0,1) << " " << bb(1,1) << " " << bb(2,1);
            bbb.Update(bb(0,0), bb(1,0), bb(2,0), bb(0,1), bb(1,1), bb(2,1));
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
        pl->SetSize(size, size);
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
#if OCC_VERSION_MAJOR<7
    for (AIS_ListIteratorOfListOfInteractive i(loi); i.More(); i.Next())
      {
          Handle_AIS_InteractiveObject o = i.Value();
#else
    for (auto i=loi.cbegin(); i!=loi.cend(); i++)
      {
          Handle_AIS_InteractiveObject o = *i;
#endif
        updatePlaneSize(o, size);
    }
}

void QoccViewWidget::onShow(QDisplayableModelTreeItem* di)
{
  insight::CurrentExceptionContext ec("showing model tree item in viewer");
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
  insight::CurrentExceptionContext ec("hiding model tree item in viewer");
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
  currentUserActivity_= std::make_shared<ViewWidgetMeasurePoints>(this);
}




void QoccViewWidget::onSelectPoints()
{
  currentUserActivity_= std::make_shared<ViewWidgetInsertPointIDs>(this);
}




void QoccViewWidget::onSelectEdges()
{
  currentUserActivity_= std::make_shared<ViewWidgetInsertEdgeIDs>(this);
}




void QoccViewWidget::onSelectFaces()
{
  currentUserActivity_= std::make_shared<ViewWidgetInsertFaceIDs>(this);
}




void QoccViewWidget::onSelectSolids()
{
  currentUserActivity_= std::make_shared<ViewWidgetInsertSolidIDs>(this);
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
#if OCC_VERSION_MAJOR<7
    for (AIS_ListIteratorOfListOfInteractive i(loi); i.More(); i.Next())
      {
          Handle_AIS_InteractiveObject o = i.Value();
#else
    for (auto i=loi.cbegin(); i!=loi.cend(); i++)
      {
          Handle_AIS_InteractiveObject o = *i;
#endif
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
#if OCC_VERSION_MAJOR<7
  for (AIS_ListIteratorOfListOfInteractive i(loi); i.More(); i.Next())
    {
        Handle_AIS_InteractiveObject o = i.Value();
#else
  for (auto i=loi.cbegin(); i!=loi.cend(); i++)
    {
        Handle_AIS_InteractiveObject o = *i;
#endif
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

///*!
//  \brief	This function handles left button down events from the mouse.
//*/
//void QoccViewWidget::onLeftButtonDown(  Qt::KeyboardModifiers nFlags, const QPoint point )
//{
//  myStartPoint = point;

//  if ( nFlags & ZOOMSHORTCUTKEY )
//    {
//      setMode( CurAction3d_DynamicZooming );
//    }
//  else if ( nFlags & PANSHORTCUTKEY )
//    {
//      setMode( CurAction3d_DynamicPanning );
//    }
//  else if ( nFlags & ROTATESHORTCUTKEY )
//    {
//      setMode( CurAction3d_DynamicRotation );
//      myView->StartRotation( point.x(), point.y() );
//    }  else
//    {
//      switch ( myMode )
//        {
//	case CurAction3d_Nothing:
//	  // emit pointClicked( myV3dX, myV3dY, myV3dZ );
//	  break;

//	case CurAction3d_Picking:
//	  break;

//	case CurAction3d_DynamicZooming:
//	  break;

//	case CurAction3d_WindowZooming:
//	  break;

//	case CurAction3d_DynamicPanning:
//	  break;

//	case CurAction3d_GlobalPanning:
//	  break;

//	case CurAction3d_DynamicRotation:
//	  myView->StartRotation( myStartPoint.x(), myStartPoint.y() );
//	  break;

//	default:
//	  Standard_Failure::Raise( "Incompatible Current Mode" );
//	  break;
//        }
//    }
//}

///*!
//  \brief	This function handles middle button down events from the mouse.
//*/
//void QoccViewWidget::onMiddleButtonDown(  Qt::KeyboardModifiers /*nFlags*/, const QPoint /*point*/ )
//{
///*
//  myStartPoint = point;
//  if ( nFlags & ZOOMSHORTCUTKEY )
//    {
//      setMode( CurAction3d_DynamicZooming );
//    }
//  else if ( nFlags & PANSHORTCUTKEY )
//    {
//      setMode( CurAction3d_DynamicPanning );
//    }
//  else
//    {
//      setMode( CurAction3d_DynamicRotation );
//      myView->StartRotation( point.x(), point.y() );
//    }
//*/
//}

///*!
//  \brief	This function handles right button down events from the mouse.
//*/
//void QoccViewWidget::onRightButtonDown(  Qt::KeyboardModifiers, const QPoint point )
//{
//  myStartPoint = point;
//  //  else
////    {
////      emit popupMenu ( this, point );
////    }
//}

///*!
//  \brief	This function handles left button up events from the mouse.
//  This marks the end of the gesture.
//*/
//void QoccViewWidget::onLeftButtonUp(  Qt::KeyboardModifiers nFlags, const QPoint point )
//{
//  myCurrentPoint = point;
  
//  if (
//  ( nFlags & ZOOMSHORTCUTKEY )
//  ||( nFlags & PANSHORTCUTKEY )
//  ||( nFlags & ROTATESHORTCUTKEY )
//   )
//    {
//      // Deactivates dynamic zooming
//      setMode( CurAction3d_Nothing );
//    }
//    else
//    {
//      switch( myMode )
//	{

//	case CurAction3d_Nothing:

//	  myContext_->Select(true);

//	  if (cimode_==CIM_Normal)
//	    {
//	      emit graphicalSelectionChanged(getSelectedItem(), this);
//	    }
//	  else if (cimode_==CIM_MeasurePoints)
//	    {
//	      myContext_->InitSelected();
//	      if (myContext_->MoreSelected())
//	      {
//		TopoDS_Shape v = myContext_->SelectedShape();
////		BRepTools::Dump(v, std::cout);
//		gp_Pnt p =BRep_Tool::Pnt(TopoDS::Vertex(v));
//		std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;

//		if (!measpts_p1_)
//		  {
//		    measpts_p1_=insight::cad::matconst(insight::vec3(p));
//		    emit sendStatus("Please select second point!");
//		  }
//		else if (!measpts_p2_)
//		  {
//		    measpts_p2_=insight::cad::matconst(insight::vec3(p));
//		    emit sendStatus("Measurement is created...");

//		    emit addEvaluationToModel
//			(
//			  "distance measurement",
//			  insight::cad::PostprocActionPtr
//			  (
//			    new insight::cad::Distance(measpts_p1_, measpts_p2_)
//			  ),
//			  true
//			);

//		    measpts_p1_.reset();
//		    measpts_p2_.reset();
//                    DeactivateAll(getContext(), TopAbs_VERTEX);
//		    cimode_=CIM_Normal;
//		  }
//	      }



//            }
//          else if (cimode_==CIM_InsertPointIDs)
//            {
//              myContext_->InitSelected();
//              if (myContext_->MoreSelected())
//                {
//                  TopoDS_Shape v = myContext_->SelectedShape();
//                  TopoDS_Vertex vv = TopoDS::Vertex(v);
//                  gp_Pnt vp = BRep_Tool::Pnt(vv);
//                  if (!selpts_)
//                    {
//                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
//                        {
//                          // restrict further selection to current shape
//                          DeactivateAll( getContext(), TopAbs_VERTEX );
//                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_VERTEX) );

//                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Vertex));

//                          insight::cad::FeatureID vid = parent->solidmodel().vertexID(v);
//                          selpts_->add(vid);
//                          emit sendStatus(boost::str(boost::format("Selected vertex %d. Select next vertex, end with right click.")%vid).c_str());
//                        }
//                    }
//                  else
//                    {
//                      insight::cad::FeatureID vid = selpts_->model()->vertexID(v);
//                      selpts_->add(vid);
//                      emit sendStatus(boost::str(boost::format("Selected vertex %d. Select next vertex, end with right click.")%vid).c_str());
//                    }
//                }
//            }
//          else if (cimode_==CIM_InsertEdgeIDs)
//            {
//              myContext_->InitSelected();
//              if (myContext_->MoreSelected())
//                {
//                  TopoDS_Shape e = myContext_->SelectedShape();
//                  if (!selpts_)
//                    {
//                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
//                        {
//                          // restrict further selection to current shape
//                          DeactivateAll( getContext(), TopAbs_EDGE );
//                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_EDGE) );

//                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Edge));

//                          insight::cad::FeatureID eid = parent->solidmodel().edgeID(e);
//                          selpts_->add(eid);
//                          emit sendStatus(boost::str(boost::format("Selected edge %d. Select next edge, end with right click.")%eid).c_str());
//                        }
//                    }
//                  else
//                    {
//                      insight::cad::FeatureID eid = selpts_->model()->edgeID(e);
//                      selpts_->add(eid);
//                      emit sendStatus(boost::str(boost::format("Selected edge %d. Select next edge, end with right click.")%eid).c_str());
//                    }
//                }
//            }
//          else if (cimode_==CIM_InsertFaceIDs)
//            {
//              myContext_->InitSelected();
//              if (myContext_->MoreSelected())
//                {
//                  TopoDS_Shape f = myContext_->SelectedShape();
//                  TopoDS_Face ff = TopoDS::Face(f);
//                  if (!selpts_)
//                    {
//                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
//                        {
//                          // restrict further selection to current shape
//                          DeactivateAll( getContext(), TopAbs_FACE );
//                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_FACE) );

//                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Face));

//                          insight::cad::FeatureID fid = parent->solidmodel().faceID(f);
//                          selpts_->add(fid);
//                          emit sendStatus(boost::str(boost::format("Selected face %d. Select next face, end with right click.")%fid).c_str());
//                        }
//                    }
//                  else
//                    {
//                      insight::cad::FeatureID fid = selpts_->model()->faceID(f);
//                      selpts_->add(fid);
//                      emit sendStatus(boost::str(boost::format("Selected face %d. Select next face, end with right click.")%fid).c_str());
//                    }
//                }
//            }
//          else if (cimode_==CIM_InsertSolidIDs)
//            {
//              myContext_->InitSelected();
//              if (myContext_->MoreSelected())
//                {
//                  TopoDS_Shape f = myContext_->SelectedShape();
//                  TopoDS_Solid ff = TopoDS::Solid(f);
//                  if (!selpts_)
//                    {
//                      if (QFeatureItem *parent=dynamic_cast<QFeatureItem*>(getOwnerItem(myContext_->SelectedInteractive())))
//                        {
//                          // restrict further selection to current shape
//                          DeactivateAll( getContext(), TopAbs_SOLID );
//                          getContext()->Activate( parent->ais(*getContext()), AIS_Shape::SelectionMode(TopAbs_SOLID) );

//                          selpts_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), insight::cad::Solid));

//                          insight::cad::FeatureID id = parent->solidmodel().solidID(f);
//                          selpts_->add(id);
//                          emit sendStatus(boost::str(boost::format("Selected solid %d. Select next solid, end with right click.")%id).c_str());
//                        }
//                    }
//                  else
//                    {
//                      insight::cad::FeatureID id = selpts_->model()->solidID(f);
//                      selpts_->add(id);
//                      emit sendStatus(boost::str(boost::format("Selected solid %d. Select next solid, end with right click.")%id).c_str());
//                    }
//                }
//            }
//	  break;

//	case CurAction3d_Picking:
//	  // Shouldn't get here yet
//	  if ( myCurrentPoint == myStartPoint )
//	    {
//	      inputEvent( nFlags & MULTISELECTIONKEY );
//	    }
//	  else
//	    {
//	      dragEvent( myStartPoint,
//			 myCurrentPoint,
//			 nFlags & MULTISELECTIONKEY );
//	    }
//	  break;

//	case CurAction3d_DynamicZooming:
//	  viewPrecision( true );
//	  break;

//	case CurAction3d_WindowZooming:
//	  if ( (abs( myCurrentPoint.x() - myStartPoint.x() ) > ValZWMin ) ||
//	       (abs( myCurrentPoint.y() - myStartPoint.y() ) > ValZWMin ) )
//	    {
//	      myView->WindowFitAll( myStartPoint.x(),
//				    myStartPoint.y(),
//				    myCurrentPoint.x(),
//				    myCurrentPoint.y() );
//	    }
//	  viewPrecision( true );
//	  break;

//	case CurAction3d_DynamicPanning:
//	  break;

//	case CurAction3d_GlobalPanning :
//	  myView->Place( myCurrentPoint.x(), myCurrentPoint.y(), myCurZoom );
//	  break;

//	case CurAction3d_DynamicRotation:
//	  break;

//	default:
//	  Standard_Failure::Raise(" Incompatible Current Mode ");
//	  break;
//	}
//    }
//}
///*!
//  \brief	Middle button up event handler.
//  This marks the end of the gesture.
//*/
//void QoccViewWidget::onMiddleButtonUp(  Qt::KeyboardModifiers /* nFlags */, const QPoint /* point */ )
//{
// // setMode( CurAction3d_Nothing );
//}

///*!
//  \brief	Right button up event handler.
//  This marks the end of the gesture.
//*/
//void QoccViewWidget::onRightButtonUp(  Qt::KeyboardModifiers, const QPoint point )
//{
//  myCurrentPoint = point;
//  /*
//  if ( nFlags & CASCADESHORTCUTKEY )
//    {
//      setMode( CurAction3d_Nothing );
//    }
//    else*/
//    {
//      if ( myMode == CurAction3d_Nothing )
//	{
//	  if (cimode_==CIM_Normal)
//	    {
//	      displayContextMenu(point);
//	    }
//	  else if (cimode_==CIM_InsertPointIDs)
//	    {
//	      QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?vid=(";
//	      int j=0;
//	      for (insight::cad::FeatureID i: selpts_->data())
//		{
//		  text+=QString::number( i );
//		  if (j++ < selpts_->size()-1) text+=",";
//		}
//	      text+=")\n";
//	      emit insertNotebookText(text);

//              DeactivateAll( getContext(), TopAbs_VERTEX );
//	      cimode_=CIM_Normal;
//	    }
//	  else if (cimode_==CIM_InsertEdgeIDs)
//	    {
//	      QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?eid=(";
//	      int j=0;
//	      for (insight::cad::FeatureID i: selpts_->data())
//		{
//		  text+=QString::number( i );
//		  if (j++ < selpts_->size()-1) text+=",";
//		}
//	      text+=")\n";
//	      emit insertNotebookText(text);

//              DeactivateAll( getContext(), TopAbs_EDGE );
//	      cimode_=CIM_Normal;
//	    }
//	  else if (cimode_==CIM_InsertFaceIDs)
//	    {
//	      QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?fid=(";
//	      int j=0;
//	      for (insight::cad::FeatureID i: selpts_->data())
//		{
//		  text+=QString::number( i );
//		  if (j++ < selpts_->size()-1) text+=",";
//		}
//	      text+=")\n";
//	      emit insertNotebookText(text);

//              DeactivateAll( getContext(), TopAbs_FACE );
//	      cimode_=CIM_Normal;
//	    }
//          else if (cimode_==CIM_InsertSolidIDs)
//            {
//              QString text = QString::fromStdString(selpts_->model()->featureSymbolName()) +"?sid=(";
//              int j=0;
//              for (insight::cad::FeatureID i: selpts_->data())
//                {
//                  text+=QString::number( i );
//                  if (j++ < selpts_->size()-1) text+=",";
//                }
//              text+=")\n";
//              emit insertNotebookText(text);

//              DeactivateAll( getContext(), TopAbs_SOLID );
//              cimode_=CIM_Normal;
//            }	  //	  emit popupMenu ( this, point );
//        }
//      else
//	{
//	  setMode( CurAction3d_Nothing );
//	}
//    }
//}

///*!
//  \brief	Mouse move event handler.
//  \param	buttons
//  \param	nFlags
//  \param	point
//  \return Nothing
//*/
//void QoccViewWidget::onMouseMove( Qt::MouseButtons buttons,
//				  Qt::KeyboardModifiers nFlags,
//				  const QPoint point,
//     Qt::KeyboardModifiers curFlags )
//{
//  myCurrentPoint = point;

//// cout<< ( curFlags & ZOOMSHORTCUTKEY )<<" "<< ( curFlags & PANSHORTCUTKEY )<<" "<<( curFlags & ROTATESHORTCUTKEY ) <<curFlags<<myMode<<endl;
  
////   if ( buttons & Qt::LeftButton  ||
////        buttons & Qt::RightButton ||
////        buttons & Qt::MidButton )
//    {
//      switch ( myMode )
//	{
//	case CurAction3d_Nothing:
//	  if ( curFlags & ZOOMSHORTCUTKEY )
//	  {
//	    myStartPoint = point;
//	    setMode(CurAction3d_DynamicZooming);
//	  }
//	  else if ( curFlags & PANSHORTCUTKEY )
//	  {
//	    myStartPoint = point;
//	    setMode(CurAction3d_DynamicPanning);
//	  }
//	  else if ( curFlags & ROTATESHORTCUTKEY )
//	  {
//	    myStartPoint = point;
//	    setMode(CurAction3d_DynamicRotation);
//	    myView->StartRotation( point.x(), point.y() );
//	  }
//	  break;

//	case CurAction3d_Picking:
//	  if ( buttons & Qt::LeftButton)
//	  {
//	  // Shouldn't get here yet
//	  drawRubberBand ( myStartPoint, myCurrentPoint );
//	  dragEvent( myStartPoint, myCurrentPoint, nFlags & MULTISELECTIONKEY );
//	  break;
//	  }

//	case CurAction3d_DynamicZooming:
//	  myView->Zoom(	myStartPoint.x(),
//			myStartPoint.y(),
//			myCurrentPoint.x(),
//			myCurrentPoint.y() );
//	  viewPrecision( true );
//	  myStartPoint = myCurrentPoint;
//	  if ( !(curFlags & ZOOMSHORTCUTKEY) ) setMode(CurAction3d_Nothing);
//	  break;

//	case CurAction3d_WindowZooming:
//	  if ( buttons & Qt::LeftButton)
//	  {
//	  drawRubberBand ( myStartPoint, myCurrentPoint );
//	  }
//	  break;

//	case CurAction3d_DynamicPanning:
//	  myView->Pan( myCurrentPoint.x() - myStartPoint.x(),
//		       myStartPoint.y() - myCurrentPoint.y() );
//	  myStartPoint = myCurrentPoint;
//	  if ( !(curFlags & PANSHORTCUTKEY) ) setMode(CurAction3d_Nothing);
//	  break;

//	case CurAction3d_GlobalPanning:
//	  break;

//	case CurAction3d_DynamicRotation:
//	  myView->Rotation( myCurrentPoint.x(), myCurrentPoint.y() );
//	  if ( !(curFlags & ROTATESHORTCUTKEY) ) setMode(CurAction3d_Nothing);
//	  break;

//	default:
//	  Standard_Failure::Raise( "Incompatible Current Mode" );
//	  break;
//	}
//    }
////   else
//    {
//      moveEvent( myCurrentPoint );
//    }
//}


///*!
//  \brief	Move event detection handler
//*/
//AIS_StatusOfDetection QoccViewWidget::moveEvent( QPoint point )
//{
//  AIS_StatusOfDetection status;
//  status = myContext_->MoveTo( point.x(), point.y(), myView
//#if (OCC_VERSION_MAJOR>=7)
//   , true
//#endif
//);
//  return status;
//}

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
//AIS_StatusOfPick QoccViewWidget::inputEvent( bool multi )
//{
//  AIS_StatusOfPick pick = AIS_SOP_NothingSelected;

//  if (multi)
//    {
//      pick = myContext_->ShiftSelect(
//#if (OCC_VERSION_MAJOR>=7)
//                    true
//#endif
//            );
//    }
//  else
//    {
//      pick = myContext_->Select(
//#if (OCC_VERSION_MAJOR>=7)
//                    true
//#endif
//            );
//    }
//  if ( pick != AIS_SOP_NothingSelected )
//    {
//      emit graphicalSelectionChanged(getSelectedItem(), this);
//    }
//  return pick;
//}

bool QoccViewWidget::dump(Standard_CString theFile)
{
//  redraw();
  return myView->Dump(theFile);
}

///*!
//  \brief This function sets the current cursor for the given interraction mode.
//  \param mode		The interraction mode
//*/
//void QoccViewWidget::setMode( const CurrentAction3d mode )
//{
//  if ( mode != myMode )
//    {
//      switch( mode )
//	{
//	case CurAction3d_DynamicPanning:
//	  setCursor( Qt::SizeAllCursor );
//	  break;
//	case CurAction3d_DynamicZooming:
//	  setCursor( Qt::CrossCursor );
//	  break;
//	case CurAction3d_DynamicRotation:
//	  setCursor( Qt::CrossCursor );
//	  break;
//	case CurAction3d_GlobalPanning:
//	  setCursor( Qt::CrossCursor );
//	  break;
//	case CurAction3d_WindowZooming:
//	  setCursor( Qt::PointingHandCursor );
//	  break;
//	case CurAction3d_Nothing:
//	  //setCursor( myCrossCursor );
//	  setCursor( Qt::CrossCursor );
//	  break;
//	default:
//	  setCursor( Qt::ArrowCursor );
//	  break;
//	}
//      myMode = mode;
//    }
//}

/*!
  \brief This is a Utility function for rounding the input value to a specific DP
*/
Standard_Real QoccViewWidget::precision( Standard_Real aReal )
{
  Standard_Real preciseReal;
  Standard_Real thePrecision = std::max (myPrecision, viewPrecision());
	
  if ( myPrecision != 0.0 )
    {
      preciseReal =  ( aReal < 0. ? -1 : (aReal > 0. ? 1 : 0.)) * floor((std::abs(aReal) + thePrecision * 0.5) / thePrecision) * thePrecision;
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


///*!
//  \brief	Draws the rubberband box
//  This function is designed to reduce "flicker" as the box is redrawn,
//  especially when the origin in the bottom corner of the window
//  \param	origin		A QPoint defining the screen origin
//  \param	position	A QPoint defining the current cursor screen location
//*/
//void QoccViewWidget::drawRubberBand( const QPoint origin, const QPoint position )
//{
//  if ( myRubberBand )
//    {
////      redraw();
//      hideRubberBand();
//      myRubberBand->setGeometry( QRect( origin, position ).normalized() );
//      showRubberBand();
//    }
//}
///*!
//  \brief	Shows the rubberband box
//*/
//void QoccViewWidget::showRubberBand( void )
//{
//  if ( myRubberBand )
//    {
//      myRubberBand->show();
//    }
//}
///*!
//  \brief	Hides the rubberband box
//*/
//void QoccViewWidget::hideRubberBand( void )
//{
//  if ( myRubberBand )
//    {
//      myRubberBand->hide();
//    }
//}

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

//OCCViewScreenshots::OCCViewScreenshots(/*Handle_AIS_InteractiveContext& context, */QString initPath)
//: QDialog(NULL)
//{
//  format_ = "pnm";
//  initialPath_ = initPath;

//  QVBoxLayout *l = new QVBoxLayout(this);
//  //setModal(false);
//  resize(1000,500);

//  occWidget_ = new QoccViewWidget(this/*, context*/);
//  occWidget_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

//  l->addWidget(occWidget_);
//  QFrame* frame = new QFrame(this);
//  frame->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
//  l->addWidget(frame);

//  QHBoxLayout *h = new QHBoxLayout(frame);

//  QPushButton *closeBtn = new QPushButton("close");
//  h->addWidget(closeBtn);
//  QPushButton *okBtn = new QPushButton("screen shot");
//  h->addWidget(okBtn);

//  connect(okBtn, &QPushButton::clicked, this, &OCCViewScreenshots::screenShot);
//  connect(closeBtn, &QPushButton::clicked, this, &OCCViewScreenshots::accept);
//}

//void OCCViewScreenshots::screenShot()
//{
//  QFileInfo fileName;
//  fileName.setFile( QFileDialog::getSaveFileName(NULL, QObject::tr("Save As"),
//                            initialPath_,
//                            QObject::tr("%1 Files (*.%2);;All Files (*)")
//                            .arg(format_.toUpper())
//                            .arg(format_)) );

//  files.append(fileName.absoluteFilePath());
//  cout << "Exporting screenshot to: " << fileName.absoluteFilePath().toStdString() << endl;
//  char fName[fileName.absoluteFilePath().length()+10];
//  strcpy(fName, fileName.absoluteFilePath().toStdString().c_str());
//  const Handle_V3d_View& myView = occWidget_->getView();
//  myView->Dump(fName);
//}








