#ifndef QOCCVIEWWIDGET_H
#define QOCCVIEWWIDGET_H

#include "toolkit_gui_export.h"

#include <memory>

#include <QWidget>
#include <QAction>
#include <QDialog>

#include "Standard_Version.hxx"
#include "V3d_View.hxx"
#include "V3d_Plane.hxx"
#include "AIS_InteractiveContext.hxx"
#include "AIS_Plane.hxx"
#include "V3d_Coordinate.hxx"


#if OCC_VERSION_MAJOR>=7
#include "AIS_ViewController.hxx"
class Aspect_GraphicCallbackStruct;
#endif

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/navigationmanager.h"
#include "iqcadmodel3dviewer/qwidgettoinputreceiveradapter.h"

#if OCC_VERSION_MAJOR>=7
#include "AIS_ViewController.hxx"


class Aspect_GraphicCallbackStruct;
#endif

class QDisplayableModelTreeItem;
class QModelTree;
class QRubberBand;
class OpenGl_GraphicDriver;
class V3d_Viewer;
class Xw_Window;

namespace insight { namespace cad {
class PostprocAction;
class Vector;
class FeatureSet;
} }

#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR>=7))
#include "Graphic3d_ClipPlane.hxx"
#endif

/** the key for multi selection */
//#define MULTISELECTIONKEY  Qt::ShiftModifier

/** The key for shortcut ( use to activate dynamic rotation, panning ) */
//#define CASCADESHORTCUTKEY Qt::ControlModifier 
//#define ZOOMSHORTCUTKEY Qt::ControlModifier
//#define PANSHORTCUTKEY Qt::ShiftModifier
//#define ROTATESHORTCUTKEY Qt::AltModifier

/* For elastic bean selection */
const double ValZWMin = 1;




class OCCViewWidgetRotation
    : public ViewWidgetAction<QoccViewWidget>
{
public:
    OCCViewWidgetRotation(
        QoccViewWidget &viewWidget, const QPoint point);

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags ) override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;
};


class OCCViewWidgetPanning
    : public ViewWidgetAction<QoccViewWidget>
{
public:
    OCCViewWidgetPanning(
        QoccViewWidget &viewWidget, const QPoint point);

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags ) override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;
};


class OCCViewWidgetDynamicZooming
    : public ViewWidgetAction<QoccViewWidget>
{
public:
    OCCViewWidgetDynamicZooming(
        QoccViewWidget &viewWidget, const QPoint point);

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags ) override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;
};



class OCCViewWidgetWindowZooming : public ViewWidgetAction<QoccViewWidget>
{
    QRubberBand *rb_;
public:
    OCCViewWidgetWindowZooming(
        QoccViewWidget &viewWidget, const QPoint point, QRubberBand* rb);
    // ~OCCViewWidgetWindowZooming();

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags ) override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;
};




class OCCViewWidgetMeasurePoints
    : public ViewWidgetAction<QoccViewWidget>
{
    std::shared_ptr<insight::cad::Vector> p1_, p2_;

public:
    OCCViewWidgetMeasurePoints(
        QoccViewWidget &viewWidget);
    // ~OCCViewWidgetMeasurePoints();

    QString description() const override;

    void start() override;

    bool onMouseClick(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;
};




class TOOLKIT_GUI_EXPORT QoccViewWidget
: public QWidgetToInputReceiverAdapter<
          QoccViewWidget, QWidget>
#if OCC_VERSION_MAJOR>=7
  , protected AIS_ViewController
#endif
{

  Q_OBJECT

private:
  std::vector<Handle_V3d_Light> lights_;

    mutable bool lastClickWasDoubleClick_;

  void addLights();

//  InputReceiver<QoccViewWidget>::Ptr currentNavigationAction_;
  // NavigationManager<QoccViewWidget>::Ptr navigationManager_;

public:


//  enum CurrentAction3d
//  {
//    CurAction3d_Undefined,
//    CurAction3d_Nothing,
//    CurAction3d_Picking,
//    CurAction3d_DynamicZooming,
//    CurAction3d_WindowZooming,
//    CurAction3d_DynamicPanning,
//    CurAction3d_GlobalPanning,
//    CurAction3d_DynamicRotation,
//  };

//  enum CurrentInteractionMode
//  {
//    CIM_Normal,
//    CIM_MeasurePoints,
//    CIM_InsertPointIDs,
//    CIM_InsertEdgeIDs,
//    CIM_InsertFaceIDs,
//    CIM_InsertSolidIDs
//  };

  struct FocusObject
  {
    Handle_AIS_InteractiveObject ais;
    bool was_visible;
  };

protected:
  std::vector<Handle_AIS_InteractiveObject> additionalDisplayObjectsForSelection_;

  std::shared_ptr<FocusObject> focussedObject;

public:

  QoccViewWidget(QWidget *parent = nullptr);
  ~QoccViewWidget();


  inline Handle_AIS_InteractiveContext&	getContext()
  { return myContext_; }

  inline const Handle_V3d_View& getView() const
  { return myView; }

  inline V3d_View& view()
  { return *myView; }

  inline const Handle_V3d_View& getOccView() const
  { return myView; }

  QPaintEngine* paintEngine() const override;

  QDisplayableModelTreeItem* getOwnerItem(Handle_AIS_InteractiveObject selected);
  QDisplayableModelTreeItem* getSelectedItem();

  void connectModelTree(QModelTree* mt) const;

  QSize	sizeHint() const override;

Q_SIGNALS:

  void initialized();
  void graphicalSelectionChanged(QDisplayableModelTreeItem* selection, QoccViewWidget* viewer);
  void mouseMoved   ( V3d_Coordinate X, V3d_Coordinate Y, V3d_Coordinate Z );
  void pointClicked ( V3d_Coordinate X, V3d_Coordinate Y, V3d_Coordinate Z );
  void sendStatus   ( const QString aMessage, double timeout=0 );
  //! Just a placeholder for now
  void error ( int errorCode, QString& errorDescription );

  void addEvaluationToModel (const QString& name, std::shared_ptr<insight::cad::PostprocAction> smp, bool visible);

  void insertNotebookText(const QString& text);

protected Q_SLOTS:
  
  void onGraphicalSelectionChanged(QDisplayableModelTreeItem* selection, QoccViewWidget* viewer);

public Q_SLOTS:
  
//  void idle();
  void fitExtents();
  void fitAll();
//  void fitArea();
//  void zoom();
//  void pan();
//  void globalPan();
//  void rotation();
  void hiddenLineOn();
  void hiddenLineOff();
  void background();
  void viewFront();
  void viewBack();
  void viewTop();
  void viewBottom();
  void viewLeft();
  void viewRight();
  void viewAxo();
  void viewTopFront();
  void viewGrid();
  void viewReset();
  void setReset();
  
  void toggleGrid ();
  void toggleClipXY ( void );
  void toggleClipYZ ( void );
  void toggleClipXZ ( void );
  void toggleClip ( double px, double py, double pz, double nx, double ny, double nz );
  
  void displayMessage(const QString& msg);

  Bnd_Box sceneBoundingBox() const;
  bool updatePlaneSize(const Handle_AIS_InteractiveObject& ppl, double size);
  void updatePlanesSizes();

  void onShow(QDisplayableModelTreeItem* di);
  void onHide(QDisplayableModelTreeItem* di);
  void onSetDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm);
  void onSetColor(QDisplayableModelTreeItem* di, Quantity_Color c);
  void onSetResolution(QDisplayableModelTreeItem* di, double res);

  void onSetClipPlane(QObject* datumplane);

  void onMeasureDistance();
  void onMeasureDiameter();
  void onSelectPoints();
  void onSelectEdges();
  void onSelectFaces();
  void onSelectSolids();

  void onUnfocus();
  void doUnfocus(bool newFocusIntended = false);
  void onFocus(Handle_AIS_InteractiveObject di);

  inline double getScale() const { return getView()->Scale(); }
  inline void setScale(double s) { view().SetScale(s); }
  inline bool pickAtCursor(bool extendSelection)
  {
      AIS_StatusOfPick picked = AIS_SOP_NothingSelected;
      if (extendSelection)
      {
        picked = getContext()->ShiftSelect(
    #if (OCC_VERSION_MAJOR>=7)
                      true
    #endif
              );
      }
      else
      {
        picked = getContext()->Select(
    #if (OCC_VERSION_MAJOR>=7)
                      true
    #endif
              );
      }
      return picked != AIS_SOP_NothingSelected;
  }

  inline void emitGraphicalSelectionChanged()
  {
      Q_EMIT graphicalSelectionChanged(
                  getSelectedItem(),
                  this );
  }

protected: // methods

  void paintEvent        ( QPaintEvent* e ) override;
  void resizeEvent       ( QResizeEvent* e ) override;
  void mouseMoveEvent    ( QMouseEvent* e ) override;

  void displayContextMenu( const QPoint& p);


private: // members

  Handle(Aspect_Window)             hWnd;
  
  Handle_V3d_View                 myView;
  Handle_V3d_Viewer               myViewer;
  Handle_AIS_InteractiveContext   myContext_;

#if ((OCC_VERSION_MAJOR<7)&&(OCC_VERSION_MINOR<7))
  Handle_V3d_Plane		clipPlane_;
#else
  Handle_Graphic3d_ClipPlane	clipPlane_;  
#endif

  double                        myCurZoom;
  bool                          myGridSnap;
  AIS_StatusOfDetection		myDetection;
  
  V3d_Coordinate					
                                myV3dX,
                                myV3dY,
                                myV3dZ;

  
  double			myPrecision;
  double			myViewPrecision;
  bool                          myMapIsValid;
  Qt::MouseButton		myButtonFlags;
  QCursor			myCrossCursor;
  
  bool                          showGrid;

//  CurrentInteractionMode        cimode_;

  // data for measure points
  std::shared_ptr<insight::cad::Vector> measpts_p1_, measpts_p2_;

  // for pointIDs
  std::shared_ptr<insight::cad::FeatureSet> selpts_;

private: // methods
  


  AIS_StatusOfPick dragEvent 
    (
     const QPoint startPoint, 
     const QPoint endPoint, 
     const bool multi = false 
     );

//  AIS_StatusOfPick inputEvent( const bool multi = false );
//  AIS_StatusOfDetection	moveEvent ( const QPoint point );
  
//  void setMode( const CurrentAction3d mode );
    
  Standard_Real precision( Standard_Real aReal );
  Standard_Real viewPrecision( bool resized = false );
  
  void drawRubberBand( const QPoint origin, const QPoint position );
  void showRubberBand( void );
  void hideRubberBand( void );
  
  Standard_Boolean convertToPlane
    (
     const Standard_Integer Xs, 
     const Standard_Integer Ys, 
     Standard_Real& X,
     Standard_Real& Y,
     Standard_Real& Z
     );
  
#if (OCC_VERSION_MAJOR<7)
  void paintOCC();
  static int paintCallBack 
    (
     Aspect_Drawable, 
     void*, 
     Aspect_GraphicCallbackStruct*
     );
#endif
    
public:

  bool dump(Standard_CString theFile);

};

////----------------------------------------------------------------------------------------------------------

//class OCCViewScreenshots : public QDialog
//{
//  Q_OBJECT
//protected:
//  QoccViewWidget* occWidget_;
//  QString format_, initialPath_;

//public:
//  QStringList files;

//  OCCViewScreenshots(/*Handle_AIS_InteractiveContext& context, */QString initPath );

//public slots:
//  void screenShot(void);

//};

#endif // QoccViewWidget_H
