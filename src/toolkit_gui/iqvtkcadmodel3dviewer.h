#ifndef IQVTKCADMODEL3DVIEWER_H
#define IQVTKCADMODEL3DVIEWER_H


#include "toolkit_gui_export.h"
#include "iqcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/iqvtkviewerstate.h"
#include "iqcadmodel3dviewer/selectionlogic.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/navigationmanager.h"

#include "vtkVersionMacros.h"
#include "vtkGenericOpenGLRenderWindow.h"
#if VTK_MAJOR_VERSION>=8
//#include <QVTKOpenGLWidget.h>
#include <QVTKOpenGLNativeWidget.h>
#else
#include <QVTKWidget.h>
#endif
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkDataObject.h>
#include <vtkPlaneCollection.h>
#include "vtkMatrix4x4.h"
#include "vtkPolyDataSilhouette.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkImageActor.h"
#include "vtkCaptionActor2D.h"

#include <QAbstractItemModel>
#include <QTimer>
#include <QItemSelectionModel>
#include <QToolButton>


#include "cadtypes.h"
#include "sketch.h"

#include <unordered_map>


class QTextEdit;
class vtkRenderWindowInteractor;
class IQVTKSelectCADEntity;
class IQVTKSelectSubshape;
class IQVTKCADModel3DViewer;

typedef
#if VTK_MAJOR_VERSION>=8
QVTKOpenGLNativeWidget
#else
QVTKWidget
#endif
VTKWidget;


class MyVTKWidget : public VTKWidget
{
    Q_OBJECT

public:
    MyVTKWidget(QWidget* parent);
    void leaveEvent(QEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

Q_SIGNALS:
    void mouseLeavesViewer();
};



class BackgroundImage
    : public QObject
{
    Q_OBJECT

    QString label_;
    boost::filesystem::path imageFileName_;
    vtkSmartPointer<vtkImageActor> imageActor_;
    vtkRenderer *usedRenderer_;

    IQVTKCADModel3DViewer& viewer();

public:
    BackgroundImage(
        const boost::filesystem::path& fp,
        IQVTKCADModel3DViewer& viewer );
    BackgroundImage(
        const rapidxml::xml_node<>& node,
        IQVTKCADModel3DViewer& viewer );
    ~BackgroundImage();

    QString label() const;

    void write(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node ) const;

public Q_SLOTS:
    void toggleVisibility(bool show);
};


class TOOLKIT_GUI_EXPORT IQVTKCADModel3DViewer
        : public IQCADModel3DViewer
{
    Q_OBJECT

public:

    friend class IQVTKSelectCADEntity;
    friend class IQVTKSelectSubshape;

    struct DisplayedEntity
    {
        QString label_;
        CADEntity ce_;
        std::vector<vtkSmartPointer<vtkProp> > actors_;
    };


    typedef std::unordered_map<
        QPersistentModelIndex,
        DisplayedEntity,
        QPersistentModelIndexHash > DisplayedData;

    typedef std::shared_ptr<IQVTKViewerState> HighlightingHandle;
    typedef std::set<
        HighlightingHandle,
        std::owner_less<HighlightingHandle> > HighlightingHandleSet;


    typedef std::map<
        vtkSmartPointer<vtkProp>,
        SubshapeData > DisplayedSubshapeData;

    class SubshapeSelection
            : public DisplayedSubshapeData
    {
        IQVTKCADModel3DViewer& viewer_;
        std::set<vtkSmartPointer<vtkProp> > temporarilyHiddenActors_;

    public:
        SubshapeSelection(IQVTKCADModel3DViewer& viewer);
        ~SubshapeSelection();

        void add(
                insight::cad::FeaturePtr feat,
                insight::cad::EntityType subshapeType );
    };

    friend class SubshapeSelection;


    class ClippingPlanes
            : public std::set<vtkSmartPointer<vtkPlane> >
    {
        IQVTKCADModel3DViewer& viewer_;
    public:
        ClippingPlanes(
                IQVTKCADModel3DViewer& viewer,
                const arma::mat& p0,
                const arma::mat& n );
        ~ClippingPlanes();

        void addToActor(vtkProp* act) const;
    };

    friend class ClippingPlanes;

    std::unique_ptr<ClippingPlanes> clipping_;


    class ViewState
    {
        IQVTKCADModel3DViewer& viewer_;
        vtkSmartPointer<vtkMatrix4x4> viewTransform_;
        double scale_;

    public:
        ViewState(IQVTKCADModel3DViewer& viewer);

        void store();
        bool hasChangedSinceUpdate() const;

        void resetCameraIfAllow();
    };

    friend class ViewState;

    static const char bgiNodeName[];

private:
    /*VTKWidget*/MyVTKWidget vtkWidget_;
    vtkSmartPointer<vtkRenderer> ren_, backgroundRen_;

    DisplayedData displayedData_;

    std::unique_ptr<SubshapeSelection> currentSubshapeSelection_;

    ViewState viewState_;

    mutable struct Bounds {
        double xmin, xmax, ymin, ymax, zmin, zmax;
        Bounds();
    } sceneBounds_;

    void recomputeSceneBounds() const;


    /**
     * @brief The HighlightItem class
     * displays a single feature exposed in red
     * and makes all others transparent
     */
    class ExposeItem : public IQVTKViewerState
    {

        CADEntity entity_;
        std::shared_ptr<DisplayedEntity> de_;
        QPersistentModelIndex idx2highlight_;

    public:
        ExposeItem(
                std::shared_ptr<DisplayedEntity> de,
                QPersistentModelIndex idx2highlight,
                IQVTKCADModel3DViewer& viewer,
                QColor hicol = QColorConstants::Red );
        ~ExposeItem();

        const CADEntity& entity() const;
        QModelIndex index() const;

    };
    friend class ExposeItem;

    mutable std::shared_ptr< ExposeItem > exposedItem_;


    /*
     * highlighting => make thick frame or something to
     * indicate that object is selected
     */

    class SilhouetteHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkPolyDataSilhouette> silhouette_;
        vtkSmartPointer<vtkActor> silhouetteActor_;

    public:
        SilhouetteHighlighter(
            IQVTKCADModel3DViewer& viewer,
            vtkPolyDataMapper* mapperToHighlight,
            QColor hicol = QColorConstants::Red );
        ~SilhouetteHighlighter();
    };
    friend class SilhouetteHighlighter;

    class LinewidthHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkActor> actor_;
        double oldColor_[3];
        float oldLineWidth_;

    public:
        LinewidthHighlighter(
            IQVTKCADModel3DViewer& viewer,
            vtkActor* actorToHighlight,
            QColor hicol = QColorConstants::Red );
        ~LinewidthHighlighter();
    };
    friend class LinewidthHighlighter;



    class PointSizeHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkActor> actor_;
        double oldColor_[3];
        float oldPointSize_;

    public:
        PointSizeHighlighter(
            IQVTKCADModel3DViewer& viewer,
            vtkActor* actorToHighlight,
            QColor hicol = QColorConstants::Red );
        ~PointSizeHighlighter();
    };
    friend class LinewidthHighlighter;



    class TextActorHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkCaptionActor2D> actor_;
        double oldColor_[3];

    public:
        TextActorHighlighter(
            IQVTKCADModel3DViewer& viewer,
            vtkCaptionActor2D* actorToHighlight,
            QColor hicol = QColorConstants::Red );
        ~TextActorHighlighter();
    };
    friend class TextActorHighlighter;

    friend class IQVTKConstrainedSketchEditor;
    friend class IQVTKCADModel3DViewerPlanePointBasedAction;


    void remove(const QPersistentModelIndex& pidx);

    std::vector<vtkSmartPointer<vtkProp> > createActor(CADEntity entity) const;

    std::vector<vtkSmartPointer<vtkProp> > findActorsOf(const QPersistentModelIndex& pidx) const;

    vtkSmartPointer<vtkProp> createSubshapeActor(
            SubshapeData sd ) const;

    void addModelEntity(
            const QPersistentModelIndex& pidx,
            const QString& lbl,
            CADEntity entity);

    void resetVisibility(const QPersistentModelIndex& pidx);
    void resetDisplayProps(const QPersistentModelIndex& pidx);

    void doExposeItem( CADEntity item );


    NavigationManager<IQVTKCADModel3DViewer>::Ptr navigationManager_;
    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr currentAction_;

    const DisplayedEntity* findDisplayedItem(
            const CADEntity& item,
            DisplayedData::const_iterator* it=nullptr ) const;


    QTimer redrawTimer_;
    bool redrawRequestedWithinWaitPeriod_;
    void redrawNow(bool force=true);

    friend class BackgroundImage;
    QList<BackgroundImage*> backgroundImages_;

    QItemSelectionModel *defaultSelectionModel_, *customSelectionModel_;

    std::set<vtkProp*> actorsExcludedFromPicking_;

    QToolButton *clBGBtn, *addBGBtn;
    void connectBackgroundImageCommands(BackgroundImage *bgi);

private Q_SLOT:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onModelAboutToBeReset();
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);

    void addChild(const QModelIndex& idx);
    void addSiblings(const QModelIndex& idx);

Q_SIGNALS:
    void contextMenuClick(QPoint pGlob);

public:
    void closeEvent(QCloseEvent *ev) override;

public:
    IQVTKCADModel3DViewer(QWidget* parent=nullptr);
    ~IQVTKCADModel3DViewer();

    HighlightingHandle highlightActor(vtkProp* actor, QColor hicol = QColorConstants::Red);
    HighlightingHandleSet highlightActors(std::set<vtkProp*> actor, QColor hicol = QColorConstants::Red);

    vtkRenderWindow* renWin();

    void setModel(QAbstractItemModel* model) override;

    QSize sizeHint() const override;
    QPointF widgetCoordsToVTK(const QPoint& widgetCoords) const;

    void setDefaultAction();
    bool isDefaultAction();
    bool launchAction(ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr activity, bool force=true);

    const Bounds& sceneBounds() const;

    void writeViewerState(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const override;
    void restoreViewerState(rapidxml::xml_node<>& node) override;


public:
    void exposeItem( insight::cad::FeaturePtr feat ) override;
    void undoExposeItem() override;

    void onMeasureDistance() override;
    void onMeasureDiameter() override;
    void onSelectPoints() override;
    void onSelectEdges() override;
    void onSelectFaces() override;
    void onSelectSolids() override;

    void toggleClip(const arma::mat& p, const arma::mat& n) override;

    void fitAll() override;

    void view( const arma::mat& viewDir, const arma::mat& upDir ) override;

    double getScale() const override;
    void setScale(double s) override;
    bool pickAtCursor(bool extendSelection) override;
    void emitGraphicalSelectionChanged() override;

    QColor getBackgroundColor() const override;
    void setBackgroundColor(QColor c) override;

    vtkRenderWindowInteractor* interactor();
    vtkRenderer* renderer();
    vtkRenderer const* renderer() const;
    void scheduleRedraw(int maxFreq=60/*Hz*/);

    void activateSelection(insight::cad::FeaturePtr feat, insight::cad::EntityType subshapeType);
    void activateSelectionAll(insight::cad::EntityType subshapeType);
    void deactivateSubshapeSelectionAll();

    vtkProp* findActorUnderCursorAt(const QPoint& clickPos) const;
    std::vector<vtkProp*> findAllActorsUnderCursorAt(const QPoint& clickPos) const;

    typedef boost::variant<
        boost::blank,
        DisplayedSubshapeData::const_iterator,
        DisplayedData::const_iterator
    > ItemAtCursor;

    ItemAtCursor findUnderCursorAt(const QPoint& clickPos) const;

    void setSelectionModel(QItemSelectionModel *selmodel) override;

#warning unify with iqvtkviewer
    arma::mat pointInPlane3D(const gp_Ax3& plane, const arma::mat& pip2d) const;
    arma::mat pointInPlane3D(const gp_Ax3& plane, const QPoint& screenPos) const;
    arma::mat pointInPlane2D(const gp_Ax3& plane, const QPoint& screenPos) const;
    arma::mat pointInPlane2D(const gp_Ax3& plane, const arma::mat& pip3d) const;

    void onlyOneShaded(QPersistentModelIndex idx) override;
    void resetRepresentations() override;

    void doSketchOnPlane(insight::cad::DatumPtr plane) override;
    void editSketch(
        const insight::cad::ConstrainedSketch& sk,
        const insight::ParameterSet& defaultGeometryParameters,
        SetSketchEntityAppearanceCallback saac,
        SketchCompletionCallback onAccept,
        SketchCompletionCallback onCancel = [](insight::cad::ConstrainedSketchPtr) {}
        ) override;

protected:
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;


//    vtkActor* getActor(insight::cad::FeaturePtr geom);
};


#endif // IQVTKCADMODEL3DVIEWER_H
