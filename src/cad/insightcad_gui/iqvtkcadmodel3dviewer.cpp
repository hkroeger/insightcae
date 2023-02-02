#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "iscadmetatyperegistrator.h"
#include "postprocactionvisualizer.h"
#include "datum.h"

#include "iqvtkcadmodel3dviewerrotation.h"
#include "iqvtkcadmodel3dviewerpanning.h"
#include "iqvtkcadmodel3dviewermeasurepoints.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTextEdit>

#include <vtkAxesActor.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkDataSetMapper.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPlaneSource.h>
#include <vtkLineSource.h>
#include <vtkProperty.h>
#include <vtkNamedColors.h>
#include "vtkArrowSource.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPropPicker.h"

#include <vtkCubeSource.h>
#include "vtkImageData.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkPointPicker.h"

#include "ivtkoccshape.h"
#include "iqpickinteractorstyle.h"
#include "iqvtkviewwidgetinsertids.h"


#include "base/vtkrendering.h"


#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2); //if render backen is OpenGL2, it should changes to vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);




uint
IQVTKCADModel3DViewer::QPersistentModelIndexHash::operator()
( const QPersistentModelIndex& idx ) const
{
    return qHash(idx);
}





void IQVTKCADModel3DViewer::remove(const QPersistentModelIndex& pidx)
{
    if (pidx.isValid())
    {
        auto i = displayedData_.find(pidx);
        if (i!=displayedData_.end())
        {
            //if (i->second.actor_)
            for (const auto& actor: i->second.actors_)
            {
                /*i->second.actor_*/actor->SetVisibility(false);
                ren_->RemoveActor(/*i->second.actor_*/actor);
            }
            displayedData_.erase(i);
        }
    }
    scheduleRedraw();
}


std::vector<vtkSmartPointer<vtkProp> > IQVTKCADModel3DViewer::createActor(CADEntity entity) const
{
    if (const auto * vertex =
            boost::get<insight::cad::VectorPtr>(&entity))
    {
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );

        vtkNew<vtkPointSource> point;
        auto p = (*vertex)->value();
        point->SetCenter(p[0], p[1], p[2]);
        point->SetNumberOfPoints(1);
        point->SetRadius(0);
        actor->GetMapper()->SetInputConnection(point->GetOutputPort());
        auto prop=actor->GetProperty();
        prop->SetRepresentationToPoints();
        prop->SetPointSize(8);
        prop->SetColor(0, 0, 0);

        return {actor};
    }
    else if (const auto * datumPtr =
             boost::get<insight::cad::DatumPtr>(&entity))
    {
        auto datum = *datumPtr;

        vtkNew<vtkNamedColors> colors;

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetProperty()->SetColor(colors->GetColor3d("lightBlue").GetData());

        if ( datum->providesPlanarReference())
        {
            vtkNew<vtkPlaneSource> plane;
            auto pl = datum->plane();
            plane->SetCenter(pl.Location().X(), pl.Location().Y(), pl.Location().Z());
            plane->SetNormal(pl.Direction().X(), pl.Direction().Y(), pl.Direction().Z());
            actor->GetMapper()->SetInputConnection(plane->GetOutputPort());
            actor->GetProperty()->SetOpacity(0.33);
        }
        else if (datum->providesAxisReference())
        {
            auto ax = datum->axis();
            gp_Pnt p1=ax.Location();

            auto arr = insight::createArrows(
                {{
                     insight::Vector(ax.Location()),
                     insight::Vector(ax.Location().XYZ() + ax.Direction().XYZ())
                 }}, false);
            actor->GetMapper()->SetInputDataObject(arr);
        }
        else if (datum->providesPointReference())
        {
            vtkNew<vtkPointSource> point;
            auto p1 = datum->point();
            point->SetCenter(p1.X(), p1.Y(), p1.Z());
            point->SetNumberOfPoints(1);
            point->SetRadius(0);
            actor->GetMapper()->SetInputConnection(point->GetOutputPort());
            auto prop=actor->GetProperty();
            prop->SetRepresentationToPoints();
            prop->SetPointSize(8);
            prop->SetColor(1., 0, 0);
        }

        return {actor};
    }
    else if (const auto *featurePtr =
             boost::get<insight::cad::FeaturePtr>(&entity))
    {
        auto feat = *featurePtr;
        vtkNew<ivtkOCCShape> shape;
        shape->SetShape( feat->shape() );

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetMapper()->SetInputConnection(shape->GetOutputPort());

        return {actor};
    }
    else if (const auto *ppPtr =
             boost::get<insight::cad::PostprocActionPtr>(&entity))
    {
        return insight::cad::postProcActionVisualizers.createVTKRepr(*ppPtr);
    }
    else if (const auto *dsPtr =
             boost::get<vtkSmartPointer<vtkDataObject> >(&entity))
    {
        auto ds = *dsPtr;

        vtkProp *actor = nullptr;

        if (auto ids = vtkImageData::SafeDownCast(ds) )
        {
            auto act = vtkSmartPointer<vtkImageActor>::New();
            auto mapper = act->GetMapper();
            mapper->SetInputData(ids);
            ren_->AddActor(act);
            actor = act;
        }
        else if (auto pds = vtkDataSet::SafeDownCast(ds) )
        {
            auto act = vtkSmartPointer<vtkActor>::New();
            act->SetMapper( vtkSmartPointer<vtkDataSetMapper>::New() );
            auto mapper=act->GetMapper();

            mapper->SetInputDataObject(pds);
            ren_->AddActor(act);
            actor = act;
        }
        else
        {
            auto act = vtkSmartPointer<vtkActor>::New();
            act->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
            act->GetMapper()->SetInputDataObject(ds);
            ren_->AddActor(act);
            actor = act;
        }

        return {actor};
    }

    return {};
}


vtkSmartPointer<vtkProp> IQVTKCADModel3DViewer::createSubshapeActor(SubshapeData sd) const
{
    if (sd.subshapeType_==insight::cad::Vertex)
    {
        auto vertex = sd.feat->vertex(sd.id_);
        auto p = BRep_Tool::Pnt(vertex);

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );

        vtkNew<vtkPointSource> point;
        point->SetCenter(p.X(), p.Y(), p.Z());
        point->SetNumberOfPoints(1);
        point->SetRadius(0);
        actor->GetMapper()->SetInputConnection(point->GetOutputPort());

        auto prop=actor->GetProperty();
        prop->SetRepresentationToSurface();
        prop->SetPointSize(8);
        prop->SetColor(1, 0, 0);

        return actor;
    }
    else if (sd.subshapeType_==insight::cad::Edge)
    {
        auto edge = sd.feat->edge(sd.id_);
        vtkNew<ivtkOCCShape> shape;
        shape->SetShape( edge );

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetMapper()->SetInputConnection(shape->GetOutputPort());
        auto prop=actor->GetProperty();
        prop->SetRepresentationToWireframe();
        prop->SetLineWidth(4);
        prop->SetColor(1, 0, 0);

        return actor;
    }
    else if (sd.subshapeType_==insight::cad::Face)
    {
        auto face = sd.feat->face(sd.id_);
        vtkNew<ivtkOCCShape> shape;
        shape->SetShape( face );

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetMapper()->SetInputConnection(shape->GetOutputPort());
        auto prop=actor->GetProperty();
        prop->SetRepresentationToSurface();
        prop->SetColor(1, 0, 0);

        return actor;
    }
    else if (sd.subshapeType_==insight::cad::Solid)
    {
        auto solid = sd.feat->subsolid(sd.id_);
        vtkNew<ivtkOCCShape> shape;
        shape->SetShape( solid );

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetMapper()->SetInputConnection(shape->GetOutputPort());
        auto prop=actor->GetProperty();
        prop->SetRepresentationToSurface();
        prop->SetColor(1, 0, 0);

        return actor;
    }
    return nullptr;
}


void IQVTKCADModel3DViewer::addModelEntity(
        const QPersistentModelIndex& pidx,
        const QString& lbl,
        CADEntity entity)
{
    auto actors = createActor(entity);
    if (actors.size())
    {
        for (const auto& a: actors)
            ren_->AddActor(a);
        displayedData_[pidx]={lbl, entity, actors};
        resetDisplayProps(pidx);
        viewState_.resetCameraIfAllow();
    }
    ren_->GetRenderWindow()->Render();
}

//void IQVTKCADModel3DViewer::addVertex(
//        const QPersistentModelIndex& pidx,
//        const QString& lbl,
//         insight::cad::VectorPtr loc )
//{
//    auto actor = createActor(loc);
//    ren_->AddActor(actor);
//    displayedData_[pidx]={lbl, loc, actor};
//}



//void IQVTKCADModel3DViewer::addDatum(
//        const QPersistentModelIndex& pidx,
//        const QString& lbl,
//        insight::cad::DatumPtr datum )
//{
//    auto actor = createActor(datum);
//    ren_->AddActor(actor);
//    displayedData_[pidx]={lbl, datum, actor};
//}




//void IQVTKCADModel3DViewer::addFeature(
//        const QPersistentModelIndex& pidx,
//        const QString& lbl,
//        insight::cad::FeaturePtr feat )
//{
//    auto actor = createActor(feat);
//    ren_->AddActor(actor);
//    displayedData_[pidx]={lbl, feat, actor};
//    resetFeatureDisplayProps(pidx);
//}

//void IQVTKCADModel3DViewer::addDataset(
//        const QPersistentModelIndex& pidx,
//        const QString& lbl,
//        vtkSmartPointer<vtkDataObject> ds )
//{
//    auto act = createActor(ds);
//    displayedData_[pidx]={lbl, ds, act};
//    resetDatasetColor(pidx);
//}



void IQVTKCADModel3DViewer::resetVisibility(const QPersistentModelIndex &pidx)
{
    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
    {
        for (const auto& actor: displayedData_[pidx].actors_)
        {
            if (auto act = vtkActor::SafeDownCast(/*displayedData_[pidx].actor_*/actor))
            {
                QModelIndex idx(pidx);
                auto visible = idx.siblingAtColumn(IQCADItemModel::visibilityCol)
                        .data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
                act->SetVisibility(visible);
            }
        }
    }
    scheduleRedraw();
}





void IQVTKCADModel3DViewer::resetDisplayProps(const QPersistentModelIndex& pidx)
{
    resetVisibility(pidx);

    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
    {
        auto& dd = displayedData_[pidx];
        if (boost::get<insight::cad::FeaturePtr>(&dd.ce_))
        {
            for (const auto& actor: dd.actors_)
            {
                if (auto act = vtkActor::SafeDownCast(/*displayedData_[pidx].actor_*/actor))
                {
                    QModelIndex idx(pidx);
                    if (auto *ivtkocc = ivtkOCCShape::SafeDownCast(act->GetMapper()->GetInputAlgorithm()))
                    {
                        if (pidx.isValid())
                        {
                            auto repr=  insight::DatasetRepresentation(
                                    idx.siblingAtColumn(IQCADItemModel::entityRepresentationCol)
                                    .data().toInt() );
                            std::cout<<"set repr="<<repr<<std::endl;
                            ivtkocc->SetRepresentation(repr);
                            ivtkocc->Update();
                        }
                    }
                    auto opacity = idx.siblingAtColumn(IQCADItemModel::entityOpacityCol)
                            .data().toDouble();
                    act->GetProperty()->SetOpacity(opacity);

                    auto color = idx.siblingAtColumn(IQCADItemModel::entityColorCol)
                            .data().value<QColor>();
                    act->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
                }
            }
        }
        else if (boost::get<vtkSmartPointer<vtkDataObject> >(&dd.ce_))
        {
            QModelIndex idx(pidx);

            auto fieldName = idx.siblingAtColumn(IQCADItemModel::datasetFieldNameCol).data()
                    .toString().toStdString();
            auto pointCell = insight::FieldSupport(
                        idx.siblingAtColumn(IQCADItemModel::datasetPointCellCol).data()
                        .toInt() );
            auto component = idx.siblingAtColumn(IQCADItemModel::datasetComponentCol).data()
                    .toInt();
            auto minVal = idx.siblingAtColumn(IQCADItemModel::datasetMinCol).data();
            auto maxVal = idx.siblingAtColumn(IQCADItemModel::datasetMaxCol).data();
            auto repr = idx.siblingAtColumn(IQCADItemModel::datasetRepresentationCol).data()
                    .toInt();

            for (const auto& act: dd.actors_)
            {

                if (auto actor = vtkActor::SafeDownCast(/*de.actor_*/act))
                {
                    vtkMapper* mapper=actor->GetMapper();
                    vtkDataSet *pds = mapper->GetInput();

                    actor->GetProperty()->SetRepresentation(repr);

                    if (fieldName.empty())
                    {
                        if (pointCell==insight::Point)
                        {
                            if (auto *arr = pds->GetPointData()->GetArray(0))
                            {
                                fieldName = arr->GetName();
                            }
                        }
                        else if (pointCell==insight::Cell)
                        {
                            if (auto *arr = pds->GetCellData()->GetArray(0))
                            {
                                fieldName = arr->GetName();
                            }
                        }
                    }

                    if (!fieldName.empty())
                    {
                        double mima[2]={0,1};
                        if (pointCell==insight::Point)
                        {
                            pds->GetPointData()->GetArray(fieldName.c_str())->GetRange(mima, component);
                        }
                        else if (pointCell==insight::Cell)
                        {
                            pds->GetCellData()->GetArray(fieldName.c_str())->GetRange(mima, component);
                        }

                        if (minVal.isValid()) mima[0]=minVal.toDouble();
                        if (maxVal.isValid()) mima[1]=maxVal.toDouble();

                        mapper->SetInterpolateScalarsBeforeMapping(true);
                        mapper->SetLookupTable( insight::createColorMap() );
                        mapper->ScalarVisibilityOn();
                        mapper->SelectColorArray(fieldName.c_str());
                        mapper->SetScalarRange(mima[0], mima[1]);
                    }
                }
            }
        }
    }
    scheduleRedraw();
}








void IQVTKCADModel3DViewer::onDataChanged(
        const QModelIndex &topLeft,
        const QModelIndex &bottomRight,
        const QVector<int> &roles )
{
    if (roles.empty() || roles.indexOf(Qt::CheckStateRole)>=0 || roles.indexOf(Qt::EditRole)>=0)
    {
        for (int r=topLeft.row(); r<=bottomRight.row(); ++r)
        {
            auto idx = model_->index(r, 0, topLeft.parent());
            QPersistentModelIndex pidx(idx);

            auto lbl = idx.siblingAtColumn(IQCADItemModel::labelCol).data().toString();
            auto feat = idx.siblingAtColumn(IQCADItemModel::entityCol).data();

            auto colInRange = [&](int minCol, int maxCol=-1)
            {
                if (maxCol<0) maxCol=minCol;
                return (topLeft.column() >= minCol)
                        &&
                       (bottomRight.column() <= maxCol);
            };

            if (roles.indexOf(Qt::EditRole)>=0 || roles.empty())
            {
                if (colInRange(IQCADItemModel::entityCol))
                {
                    // exchange feature
                    remove( pidx );
                    addChild(idx);
                }
                if ( colInRange(IQCADItemModel::datasetFieldNameCol,
                                IQCADItemModel::datasetRepresentationCol) )
                {
                    resetDisplayProps(pidx);
                }
            }

            if (roles.indexOf(Qt::CheckStateRole)>=0 || roles.empty())
            {
                if (colInRange(IQCADItemModel::visibilityCol))
                {
                    resetVisibility(pidx);
                }
            }

        }
    }
}




void IQVTKCADModel3DViewer::onModelAboutToBeReset()
{}




void IQVTKCADModel3DViewer::onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{}




void IQVTKCADModel3DViewer::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    for (int r=start; r<=end; ++r)
    {
        addChild( model_->index(r, 0, parent) );
    }
}




void IQVTKCADModel3DViewer::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    for (int r=first; r<=last; ++r)
    {
        remove( model_->index(r, 0, parent) );
    }
}



void IQVTKCADModel3DViewer::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
}


void IQVTKCADModel3DViewer::addChild(const QModelIndex& idx)
{
    QPersistentModelIndex pidx(idx);
    auto lbl = idx.siblingAtColumn(IQCADItemModel::labelCol).data().toString();
    auto feat = idx.siblingAtColumn(IQCADItemModel::entityCol).data();

    CADEntity modelEntity;
    if (feat.canConvert<insight::cad::VectorPtr>())
    {
        modelEntity = feat.value<insight::cad::VectorPtr>();
    }
    else if (feat.canConvert<insight::cad::FeaturePtr>())
    {
        modelEntity = feat.value<insight::cad::FeaturePtr>();
    }
    else if (feat.canConvert<insight::cad::DatumPtr>())
    {
        modelEntity = feat.value<insight::cad::DatumPtr>();
    }
    else if (feat.canConvert<insight::cad::PostprocActionPtr>())
    {
        modelEntity = feat.value<insight::cad::PostprocActionPtr>();
    }
    else if (feat.canConvert<vtkSmartPointer<vtkDataObject> >())
    {
        modelEntity = feat.value<vtkSmartPointer<vtkDataObject> >();
    }
    else
        return; // don't addModelEntity, if no match

    addModelEntity( pidx, lbl, modelEntity );
    resetVisibility(pidx);
}





void IQVTKCADModel3DViewer::addSiblings(const QModelIndex& idx)
{
    auto nrows=model_->rowCount(idx);
    for (int r=0; r<nrows; ++r)
    {
        auto i = model_->index(r, 1, idx);
        if (model_->rowCount(i)>0)
        {
            addSiblings(i);
        }
        else
        {
            addChild(i);
        }
    }
}




void IQVTKCADModel3DViewer::onMeasureDistance()
{
    auto md = std::make_shared<IQVTKCADModel3DViewerMeasurePoints>(*this);
    currentUserActivity_= md;
}




void IQVTKCADModel3DViewer::onSelectPoints()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertPointIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    currentUserActivity_= md;
}




void IQVTKCADModel3DViewer::onSelectEdges()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertEdgeIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    currentUserActivity_= md;
}




void IQVTKCADModel3DViewer::onSelectFaces()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertFaceIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    currentUserActivity_= md;
}




void IQVTKCADModel3DViewer::onSelectSolids()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertSolidIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    currentUserActivity_= md;
}





void IQVTKCADModel3DViewer::toggleClip(const arma::mat& p, const arma::mat& n)
{
  if (!clipping_)
  {
      clipping_.reset(new ClippingPlanes(*this, p, n));
  }
  else
  {
      clipping_.reset();
  }
}

void IQVTKCADModel3DViewer::fitAll()
{
    ren_->ResetCamera();
    scheduleRedraw();
}

void IQVTKCADModel3DViewer::view(
        const arma::mat &viewDir,
        const arma::mat &upDir )
{
    auto cam = ren_->GetActiveCamera();

    arma::mat cp, fp;
    cp=fp=insight::vec3Zero();

    cam->GetPosition(cp.memptr());
    cam->GetFocalPoint(fp.memptr());

    double L=arma::norm(fp-cp,2);
    cam->SetViewUp(upDir.memptr());
    cam->SetPosition( arma::mat(fp - viewDir*L).memptr() );

    scheduleRedraw();
}

void IQVTKCADModel3DViewer::highlightItem(insight::cad::FeaturePtr feat)
{
    doHighlightItem(feat);
}


void IQVTKCADModel3DViewer::doHighlightItem(CADEntity item)
{
    if (highlightedItem_
            && highlightedItem_->entity()==item)
        return;

    highlightedItem_.reset(); // clear existing highlighting, if any


    if (const auto* fPtr = boost::get<insight::cad::FeaturePtr>(&item))
    {
        auto feat = *fPtr;
        auto name = QString::fromStdString(feat->featureSymbolName());


        DisplayedData::const_iterator idisp;
        findDisplayedItem(item, &idisp);

        if (idisp!=displayedData_.end())
        {
            // already in display
            highlightedItem_ = std::make_shared<HighlightItem>(
                        nullptr, idisp->first, this );
        }
        else
        {
            // not in display, add temporarily
            auto actor = createActor(feat);
            highlightedItem_ = std::make_shared<HighlightItem>(
                        std::shared_ptr<DisplayedEntity>(
                            new DisplayedEntity{name, item, {actor}} ),
                        QPersistentModelIndex(),
                        this );
        }
    }
}

const IQVTKCADModel3DViewer::DisplayedEntity*
IQVTKCADModel3DViewer::findDisplayedItem(
        const CADEntity &item,
        DisplayedData::const_iterator* it ) const
{
    auto idisp = std::find_if(
                displayedData_.begin(),
                displayedData_.end(),
                [item](const DisplayedData::value_type& dd)
                  { return dd.second.ce_ == item; }
      );

    if (it) *it=idisp;

    if (idisp!=displayedData_.end())
        return &(idisp->second);
    else
        return nullptr;
}


void IQVTKCADModel3DViewer::scheduleRedraw()
{
    redrawTimer_.start(100);
}


void IQVTKCADModel3DViewer::undoHighlightItem()
{
    highlightedItem_.reset();
}


IQVTKCADModel3DViewer::IQVTKCADModel3DViewer(
        QWidget* parent )
    : IQCADModel3DViewer(parent),
      vtkWidget_(this),
      navigationManager_(
        std::make_shared<
          TouchpadNavigationManager<
           IQVTKCADModel3DViewer, IQVTKCADModel3DViewerPanning, IQVTKCADModel3DViewerRotation
          > >(currentNavigationAction_, *this) ),
      viewState_(*this)
{
    setMouseTracking( true );
    setFocusPolicy(Qt::StrongFocus);

    ren_ = vtkSmartPointer<vtkRenderer>::New();

    vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
    renderWindow
#else
    GetRenderWindow
#endif
    ()->AddRenderer(ren_);

    ren_->SetBackground(1., 1., 1.);

    insight::dbg()<<"layout"<<std::endl;
    auto btnLayout=new QHBoxLayout;
    auto lv = new QVBoxLayout;
    lv->addLayout(btnLayout);
    lv->addWidget(&vtkWidget_);
    setLayout(lv);

    insight::dbg()<<"fit btn"<<std::endl;
    auto fitBtn = new QPushButton(QPixmap(":/icons/icon_fit_all.svg"), "");
    fitBtn->setIconSize(QSize(24,24));
    btnLayout->addWidget(fitBtn);
    connect(fitBtn, &QPushButton::clicked,
            this, &IQCADModel3DViewer::fitAll );

//    auto setCam = [this](double upx, double upy, double upz,
//                         double dx, double dy, double dz)
//    {
//        auto cam = ren_->GetActiveCamera();
//        arma::mat cp, fp;
//        cp=fp=insight::vec3Zero();
//        cam->GetPosition(cp.memptr());
//        cam->GetFocalPoint(fp.memptr());
//        double L=arma::norm(fp-cp,2);
//        cam->SetViewUp(upx,upy,upz);
//        cam->SetPosition( arma::mat(fp - insight::vec3(dx,dy,dz)*L).memptr() );
//    };

    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_plusx.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked,
                this, &IQCADModel3DViewer::viewFront );
//                std::bind(setCam, 0,0,1, 1,0,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_minusx.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked,
                this, &IQCADModel3DViewer::viewBack );
//                std::bind(setCam, 0,0,1, -1,0,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_plusy.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked,
                this, &IQCADModel3DViewer::viewRight );
//                std::bind(setCam, 0,0,1, 0,1,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_minusy.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked,
                this, &IQCADModel3DViewer::viewLeft );
//                std::bind(setCam, 0,0,1, 0,-1,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_plusz.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked,
                this, &IQCADModel3DViewer::viewBottom );
//                std::bind(setCam, 0,1,0, 0,0,1));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_minusz.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked,
                this, &IQCADModel3DViewer::viewTop );
//                std::bind(setCam, 0,1,0, 0,0,-1));
    }

    btnLayout->addItem(new QSpacerItem(1,1,QSizePolicy::Expanding));
    auto axes = vtkSmartPointer<vtkAxesActor>::New();

    auto renWin1 = vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
    renderWindow
#else
    GetRenderWindow
#endif
            ();

    // Call vtkRenderWindowInteractor in orientation marker widgt
    auto widget = vtkOrientationMarkerWidget::New();
    widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes );
    widget->SetInteractor( renWin1->GetInteractor() );
    widget->SetViewport( 0.0, 0.0, 0.3, 0.3 );
    widget->SetEnabled( 1 );
    widget->InteractiveOn();

//    vtkNew<IQPickInteractorStyle> style;
//    connect(
//        style, &IQPickInteractorStyle::contextMenuRequested, this,
//        [this](vtkActor* actor)
//        {

//            auto i = std::find_if(
//                        displayedData_.begin(),
//                        displayedData_.end(),
//                        [actor](const DisplayedData::value_type& e)
//                        { return (e.second.actor_==actor); }
//            );
//            if (i!=displayedData_.end())
//            {
//                Q_EMIT contextMenuRequested(i->first, QCursor::pos());
//            }
//        }
//    );
////    vtkNew<vtkInteractorStyleTrackballCamera> style;
//    insight::dbg()<<"set default renderer"<<std::endl;
//    style->SetDefaultRenderer(ren_);

//    insight::dbg()<<"set interactor style"<<std::endl;
//    renWin1->GetInteractor()->SetInteractorStyle(style);

    ren_->GetActiveCamera()->SetParallelProjection(true);

    insight::dbg()<<"reset cam"<<std::endl;
    ren_->ResetCamera();

    viewState_.store();

    redrawTimer_.setSingleShot(true);
    connect(
        &redrawTimer_, &QTimer::timeout,
        [this]()
        {
//            ren_->GetRenderWindow()->Render();
            vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
            renderWindow
#else
            GetRenderWindow
#endif
            ()->Render();
        }
    );
}



IQVTKCADModel3DViewer::~IQVTKCADModel3DViewer()
{
    clipping_.reset();
    highlightedItem_.reset();
}




void IQVTKCADModel3DViewer::setModel(QAbstractItemModel* model)
{
    if (model_)
    {
         disconnect(model_, &QAbstractItemModel::dataChanged, 0, 0);
         disconnect(model_, &QAbstractItemModel::modelAboutToBeReset, 0, 0);
         disconnect(model_, &QAbstractItemModel::rowsAboutToBeRemoved, 0, 0);
         disconnect(model_, &QAbstractItemModel::rowsAboutToBeInserted, 0, 0);
         disconnect(model_, &QAbstractItemModel::rowsInserted, 0, 0);
    }

    IQCADModel3DViewer::setModel(model);

    connect(model, &QAbstractItemModel::dataChanged,
            this, &IQVTKCADModel3DViewer::onDataChanged);

    connect(model, &QAbstractItemModel::modelAboutToBeReset,
            this, &IQVTKCADModel3DViewer::onModelAboutToBeReset);

    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &IQVTKCADModel3DViewer::onRowsAboutToBeRemoved);

    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &IQVTKCADModel3DViewer::onRowsRemoved);

    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &IQVTKCADModel3DViewer::onRowsAboutToBeInserted);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &IQVTKCADModel3DViewer::onRowsInserted);

    addSiblings(QModelIndex());
}





vtkRenderWindowInteractor* IQVTKCADModel3DViewer::interactor()
{
    return vtkWidget_.GetInteractor();
}




vtkRenderer* IQVTKCADModel3DViewer::renderer()
{
    return ren_;
}




void IQVTKCADModel3DViewer::activateSelection(
        insight::cad::FeaturePtr feat,
        insight::cad::EntityType subshapeType )
{
    if (!currentSubshapeSelection_)
    {
        currentSubshapeSelection_.reset();
        currentSubshapeSelection_.reset(
                    new SubshapeSelection(*this) );
    }
    currentSubshapeSelection_->add(feat, subshapeType);
}

void IQVTKCADModel3DViewer::activateSelectionAll(insight::cad::EntityType subshapeType)
{
    for (const auto& disp: displayedData_)
    {
        if (auto* featPtr =
                boost::get<insight::cad::FeaturePtr>(&disp.second.ce_))
        {
            activateSelection(*featPtr, subshapeType);
        }
    }
}



void IQVTKCADModel3DViewer::deactivateSubshapeSelectionAll()
{
    currentSubshapeSelection_.reset();
}


boost::variant<
    boost::blank,
    IQVTKCADModel3DViewer::DisplayedSubshapeData::const_iterator,
    IQVTKCADModel3DViewer::DisplayedData::const_iterator
> IQVTKCADModel3DViewer::findPicked(int clickPos[]) const
{
    // Pick from this location.
    insight::dbg()<<"click pos = "<<clickPos[0]<<" "<<clickPos[1]<<std::endl;
    vtkNew<vtkPointPicker> picker;
    picker->Pick(clickPos[0], size().height()-clickPos[1]-1, 0, ren_);

    double* pos = picker->GetPickPosition();
    insight::dbg()
            << "Pick position (world coordinates) is: "
            << pos[0] << " " << pos[1] << " " << pos[2]
            << std::endl;

    if (auto pickedActor = picker->GetActor())
    {
        insight::dbg()<<" picked actor = "<<pickedActor<<std::endl;

        if (currentSubshapeSelection_)
        {
            auto ps = currentSubshapeSelection_->find(pickedActor);
            if (ps!=currentSubshapeSelection_->end())
            {
                insight::dbg()<<"picked subshape "<<ps->second.id_<<std::endl;
                return ps;
            }
        }

        auto idisp = std::find_if(
                    displayedData_.begin(),
                    displayedData_.end(),
                    [pickedActor](const DisplayedData::value_type& dd)
//                      { return dd.second.actor_ == pickedActor; }
                    { return std::find(dd.second.actors_.begin(),
                                       dd.second.actors_.end(), pickedActor)
                                        !=dd.second.actors_.end(); }
          );
        if (idisp!=displayedData_.end())
        {
            insight::dbg()<<"picked shape \""<<idisp->second.label_.toStdString()<<"\""<<std::endl;
            return idisp;
        }
    }
    else
    {
        insight::dbg()<<"nothing picked"<<std::endl;
    }


    return boost::blank();
}

void IQVTKCADModel3DViewer::onlyOneShaded(QPersistentModelIndex pidx)
{
    auto setRepr = [&](const DisplayedEntity& de, insight::DatasetRepresentation r)
    {
        for (const auto& a: de.actors_)
        {
            if (auto* act = vtkActor::SafeDownCast(a))
            {
                QModelIndex idx(pidx);
                if (auto *ivtkocc = ivtkOCCShape::SafeDownCast(
                            act->GetMapper()->GetInputAlgorithm()) )
                {
                    if (pidx.isValid())
                    {
                        auto repr=  insight::DatasetRepresentation(
                                idx.siblingAtColumn(IQCADItemModel::entityRepresentationCol)
                                .data().toInt() );
                        std::cout<<"set repr="<<repr<<std::endl;
                        ivtkocc->SetRepresentation(r);
                        ivtkocc->Update();
                    }
                }
            }
        }
    };

    for (const auto& de: displayedData_)
    {
        if (de.first==pidx)
            setRepr(de.second, insight::DatasetRepresentation::Surface);
        else
            setRepr(de.second, insight::DatasetRepresentation::Wireframe);
    }

    scheduleRedraw();
}

void IQVTKCADModel3DViewer::resetRepresentations()
{
    for (const auto& de: displayedData_)
    {
        resetDisplayProps(de.first);
    }
}

QSize IQVTKCADModel3DViewer::sizeHint() const
{
    return QSize(1024,768);
}


double IQVTKCADModel3DViewer::getScale() const
{
    vtkCamera* camera = ren_->GetActiveCamera();
    if (camera->GetParallelProjection())
    {
      return 1./camera->GetParallelScale();
    }
    else
    {
      return 1./camera->GetDistance();
    }
}

void IQVTKCADModel3DViewer::setScale(double s)
{
    vtkCamera* camera = ren_->GetActiveCamera();
    if (camera->GetParallelProjection())
    {
      camera->SetParallelScale(1./s);
    }
    else
    {
      camera->SetDistance(1./s);
      ren_->ResetCameraClippingRange();
    }

    if (interactor()->GetLightFollowCamera())
    {
      ren_->UpdateLightsGeometryToFollowCamera();
    }

    ren_->Render();
}

bool IQVTKCADModel3DViewer::pickAtCursor(bool extendSelection)
{
    return false;
}

void IQVTKCADModel3DViewer::emitGraphicalSelectionChanged()
{}


QColor IQVTKCADModel3DViewer::getBackgroundColor() const
{
    double r, g, b;
    ren_->GetBackground(r, g, b);
    QColor c;
    c.setRgbF(r, g, b);
    return c;
}

void IQVTKCADModel3DViewer::setBackgroundColor(QColor c)
{
    ren_->SetBackground(c.redF(), c.greenF(), c.blueF());
    scheduleRedraw();
}



void IQVTKCADModel3DViewer::mousePressEvent   ( QMouseEvent* e )
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




void IQVTKCADModel3DViewer::mouseReleaseEvent ( QMouseEvent* e )
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
            vtkNew<vtkPropPicker> picker;
            picker->Pick(e->pos().x(), size().height() -  e->pos().y(),
                         0, ren_ );

            auto pickedActor = picker->GetActor();

            if (pickedActor != nullptr)
            {
                auto i = std::find_if(
                            displayedData_.begin(),
                            displayedData_.end(),
                            [pickedActor](const DisplayedData::value_type& e)
//                            { return (e.second.actor_==pickedActor); }
                            { return std::find(e.second.actors_.begin(),
                                               e.second.actors_.end(), pickedActor)
                                                !=e.second.actors_.end(); }
                );
                if (i!=displayedData_.end())
                {
                  Q_EMIT contextMenuRequested(
                                i->first,
                                mapToGlobal(e->pos()) );
                }
            }
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




void IQVTKCADModel3DViewer::mouseMoveEvent    ( QMouseEvent* e )
{
    navigationManager_->onMouseMove( e->buttons(), e->pos(), e->modifiers() );
    if (currentNavigationAction_)
      currentNavigationAction_->onMouseMove( e->buttons(), e->pos(), e->modifiers() );
    if (currentUserActivity_)
      currentUserActivity_->onMouseMove( e->buttons(), e->pos(), e->modifiers() );

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();
}




void IQVTKCADModel3DViewer::wheelEvent        ( QWheelEvent* e )
{
    navigationManager_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());
    if (currentNavigationAction_)
        currentNavigationAction_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());
    if (currentUserActivity_)
        currentUserActivity_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());

    if (currentUserActivity_ && currentUserActivity_->finished())
        currentUserActivity_.reset();
}




void IQVTKCADModel3DViewer::keyPressEvent     ( QKeyEvent* e )
{
    insight::dbg()<<"key press event"<<std::endl;

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




void IQVTKCADModel3DViewer::keyReleaseEvent   ( QKeyEvent* e )
{
    insight::dbg()<<"key release event"<<std::endl;

    navigationManager_->onKeyRelease(e->modifiers(), e->key());

    if (currentNavigationAction_)
      currentNavigationAction_->onKeyRelease(e->modifiers(), e->key());

    if (currentUserActivity_)
      currentUserActivity_->onKeyRelease(e->modifiers(), e->key());

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();

    QWidget::keyReleaseEvent(e);
}



IQVTKCADModel3DViewer::ViewState::ViewState(IQVTKCADModel3DViewer& viewer)
    : viewer_(viewer),
      viewTransform_( vtkSmartPointer<vtkMatrix4x4>::New() )
{}

void IQVTKCADModel3DViewer::ViewState::store()
{
    auto *cam=viewer_.renderer()->GetActiveCamera();
    viewTransform_->DeepCopy( cam->GetModelViewTransformMatrix() );
    scale_=viewer_.getScale();
}

bool IQVTKCADModel3DViewer::ViewState::hasChangedSinceUpdate() const
{
    auto *cam=viewer_.renderer()->GetActiveCamera();
    bool equal=true;
    for (int i=0; i<4; ++i)
    {
        for (int j=0; j<4; ++j)
        {
            equal = equal &&
                    ( viewTransform_->GetElement(i,j) ==
                       cam->GetModelViewTransformMatrix()->GetElement(i,j) );
        }
    }

    equal = equal && (scale_==viewer_.getScale());

    return !equal;
}

void IQVTKCADModel3DViewer::ViewState::resetCameraIfAllow()
{
    auto *ren=viewer_.renderer();
    if (!hasChangedSinceUpdate())
    {
        ren->ResetCamera();
        viewer_.scheduleRedraw();
        store();
    }
}
