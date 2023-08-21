#ifndef IQVTKCADMODEL3DVIEWER_H
#define IQVTKCADMODEL3DVIEWER_H


#include "toolkit_gui_export.h"
#include "iqcadmodel3dviewer.h"
#include "iqvtkviewerstate.h"
//#include "iqvtkconstrainedsketcheditor.h"

#include "vtkVersionMacros.h"
#include "vtkGenericOpenGLRenderWindow.h"
#if VTK_MAJOR_VERSION>=8
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget.h>
#endif
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkDataObject.h>
#include <vtkPlaneCollection.h>
#include "vtkMatrix4x4.h"
#include "vtkPolyDataSilhouette.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkImageActor.h"

#include <QAbstractItemModel>
#include <QTimer>
#include <QItemSelectionModel>


#include "cadtypes.h"
#include "viewwidgetaction.h"
#include "navigationmanager.h"
#include "sketch.h"

#include <unordered_map>


class QTextEdit;
class vtkRenderWindowInteractor;

typedef
#if VTK_MAJOR_VERSION>=8
QVTKOpenGLWidget
#else
QVTKWidget
#endif
VTKWidget;




class TOOLKIT_GUI_EXPORT IQVTKCADModel3DViewer
        : public IQCADModel3DViewer
{
    Q_OBJECT

public:

    struct QPersistentModelIndexHash
    {
        uint operator()(const QPersistentModelIndex& idx) const;
    };

    typedef boost::variant<
        insight::cad::VectorPtr,
        insight::cad::DatumPtr,
        insight::cad::FeaturePtr,
        insight::cad::PostprocActionPtr,
        vtkSmartPointer<vtkDataObject>
    > CADEntity;


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

    struct SubshapeData {
        insight::cad::FeaturePtr feat;
        insight::cad::EntityType subshapeType_;
        insight::cad::FeatureID id_;
    };

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


private:
    VTKWidget vtkWidget_;
    vtkSmartPointer<vtkRenderer> ren_, backgroundRen_;

    DisplayedData displayedData_;

    std::unique_ptr<SubshapeSelection> currentSubshapeSelection_;

    ViewState viewState_;

    mutable struct Bounds {
        double xmin, xmax, ymin, ymax, zmin, zmax;
        Bounds();
    } sceneBounds_;

    void recomputeSceneBounds() const;


#warning should be better named "expose"

    /**
     * @brief The HighlightItem class
     * displays a single feature exposed in red
     * and makes all others transparent
     */
    class HighlightItem : public IQVTKViewerState
    {

        CADEntity entity_;
        std::shared_ptr<DisplayedEntity> de_;
        QPersistentModelIndex idx2highlight_;

    public:
        HighlightItem(
                std::shared_ptr<DisplayedEntity> de,
                QPersistentModelIndex idx2highlight,
                IQVTKCADModel3DViewer& viewer);
        ~HighlightItem();

        const CADEntity& entity() const;
        QModelIndex index() const;

    };
    friend class HighlightItem;

    mutable std::shared_ptr< HighlightItem > highlightedItem_;


    /*
     * highlighting => make thick frame or something to
     * indicate that object is selected
     */

    class SilhouetteHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkPolyDataSilhouette> silhouette_;
        vtkSmartPointer<vtkActor> silhouetteActor_;

    public:
        SilhouetteHighlighter(IQVTKCADModel3DViewer& viewer, vtkPolyDataMapper* mapperToHighlight)
            : IQVTKViewerState(viewer)
        {
            auto* id = mapperToHighlight->GetInput();
            if (id->GetNumberOfCells()>0)
            {
                silhouette_ = vtkSmartPointer<vtkPolyDataSilhouette>::New();
                silhouette_->SetCamera(viewer_.renderer()->GetActiveCamera());

                // Create mapper and actor for silhouette
                auto silhouetteMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                silhouetteMapper->SetInputConnection(silhouette_->GetOutputPort());

                silhouetteActor_ = vtkSmartPointer<vtkActor>::New();
                silhouetteActor_->SetMapper(silhouetteMapper);
                silhouetteActor_->GetProperty()->SetColor(0.5, 0.5, 0.5);
                silhouetteActor_->GetProperty()->SetLineWidth(2);

                silhouette_->SetInputData(id);
                viewer_.renderer()->AddActor(silhouetteActor_);
            }
            else if (id->GetNumberOfLines()>0)
            {
            }
        }

        ~SilhouetteHighlighter()
        {
            viewer_.renderer()->RemoveActor(silhouetteActor_);
        }

    };
    friend class SilhouetteHighlighter;

    class LinewidthHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkActor> actor_;
        double oldColor_[3];
        float oldLineWidth_;

    public:
        LinewidthHighlighter(IQVTKCADModel3DViewer& viewer, vtkActor* actorToHighlight)
            : IQVTKViewerState(viewer), actor_(actorToHighlight)
        {
            oldLineWidth_=actor_->GetProperty()->GetLineWidth();
            actor_->GetProperty()->GetColor(oldColor_);

            actor_->GetProperty()->SetLineWidth(oldLineWidth_+2);
            actor_->GetProperty()->SetColor(0.5, 0.5, 0.5);

            viewer_.scheduleRedraw();
        }

        ~LinewidthHighlighter()
        {
            actor_->GetProperty()->SetLineWidth(oldLineWidth_);
            actor_->GetProperty()->SetColor(oldColor_);

            viewer_.scheduleRedraw();
        }

    };
    friend class LinewidthHighlighter;


    class PointSizeHighlighter : public IQVTKViewerState
    {
        vtkSmartPointer<vtkActor> actor_;
        double oldColor_[3];
        float oldPointSize_;

    public:
        PointSizeHighlighter(IQVTKCADModel3DViewer& viewer, vtkActor* actorToHighlight)
            : IQVTKViewerState(viewer), actor_(actorToHighlight)
        {
            oldPointSize_=actor_->GetProperty()->GetPointSize();
            actor_->GetProperty()->GetColor(oldColor_);

            actor_->GetProperty()->SetPointSize(oldPointSize_+2);
            actor_->GetProperty()->SetColor(0.5, 0.5, 0.5);

            viewer_.scheduleRedraw();
        }

        ~PointSizeHighlighter()
        {
            actor_->GetProperty()->SetPointSize(oldPointSize_);
            actor_->GetProperty()->SetColor(oldColor_);

            viewer_.scheduleRedraw();
        }

    };
    friend class LinewidthHighlighter;

    typedef std::set< std::pair<vtkProp*, std::shared_ptr<IQVTKViewerState> > > HighlightedActors;
    HighlightedActors highlightedActors_;



    friend class IQVTKConstrainedSketchEditor;
    friend class IQVTKCADModel3DViewerPlanePointBasedAction;

//    std::unique_ptr<IQVTKConstrainedSketchEditor> displayedSketch_;



    void remove(const QPersistentModelIndex& pidx);

    std::vector<vtkSmartPointer<vtkProp> > createActor(CADEntity entity) const;

    std::vector<vtkSmartPointer<vtkProp> > findActorsOf(const QPersistentModelIndex& pidx) const;
//    std::vector<vtkSmartPointer<vtkProp> > findActorsOf(CADEntity entity) const;

    vtkSmartPointer<vtkProp> createSubshapeActor(
            SubshapeData sd ) const;

    void addModelEntity(
            const QPersistentModelIndex& pidx,
            const QString& lbl,
            CADEntity entity);

    void resetVisibility(const QPersistentModelIndex& pidx);
    void resetDisplayProps(const QPersistentModelIndex& pidx);

    void doHighlightItem( CADEntity item );


    InputReceiver<IQVTKCADModel3DViewer>::Ptr currentNavigationAction_;
    NavigationManager<IQVTKCADModel3DViewer>::Ptr navigationManager_;
    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr currentUserActivity_;

    const DisplayedEntity* findDisplayedItem(
            const CADEntity& item,
            DisplayedData::const_iterator* it=nullptr ) const;

    QTimer redrawTimer_;
    void scheduleRedraw(int millisec=100);

    class BackgroundImage : public IQVTKViewerState
    {
        vtkSmartPointer<vtkImageActor> imageActor_;
        vtkRenderer *usedRenderer_;

    public:
        BackgroundImage(
                const boost::filesystem::path& fp,
                IQVTKCADModel3DViewer& viewer );

        ~BackgroundImage();
    };
    friend class BackgroundImage;

    std::unique_ptr<BackgroundImage> backgroundImage_;

    QItemSelectionModel *defaultSelectionModel_, *customSelectionModel_;

private Q_SLOT:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onModelAboutToBeReset();
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);

    void addChild(const QModelIndex& idx);
    void addSiblings(const QModelIndex& idx);

public:
    void closeEvent(QCloseEvent *ev) override;

public:
    IQVTKCADModel3DViewer(QWidget* parent=nullptr);
    ~IQVTKCADModel3DViewer();

    typedef std::weak_ptr<IQVTKViewerState> HighlightingHandle;
    typedef std::set<
        HighlightingHandle,
        std::owner_less<HighlightingHandle> > HighlightingHandleSet;
    HighlightingHandle highlightActor(vtkProp* actor);
    HighlightingHandleSet highlightActors(std::set<vtkProp*> actor);
    void unhighlightActor(HighlightedActors::const_iterator toBeRemoved);
    void unhighlightActor(vtkProp* actor);
    void unhighlightActor(HighlightingHandle highlighter);
    void unhighlightActors(HighlightingHandleSet highlighters);

    void setBackgroundImage(const boost::filesystem::path& imageFile);
    vtkRenderWindow* renWin();

    void setModel(QAbstractItemModel* model) override;

    QSize sizeHint() const override;
    QPointF widgetCoordsToVTK(const QPoint& widgetCoords) const;

    bool launchUserActivity(ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr activity, bool force=true);

    const Bounds& sceneBounds() const;

public:
    void highlightItem( insight::cad::FeaturePtr feat ) override;
    void undoHighlightItem() override;

    void onMeasureDistance() override;
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
            insight::cad::ConstrainedSketchPtr sk,
            const insight::ParameterSet& defaultGeometryParameters,
            SetSketchEntityAppearanceCallback saac,
            SketchCompletionCallback scc ) override;

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
