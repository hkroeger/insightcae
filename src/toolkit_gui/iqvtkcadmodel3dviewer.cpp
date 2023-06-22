#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "iscadmetatyperegistrator.h"
#include "postprocactionvisualizer.h"
#include "datum.h"

#include "iqvtkcadmodel3dviewerrotation.h"
#include "iqvtkcadmodel3dviewerpanning.h"
#include "iqvtkvieweractions/iqvtkcadmodel3dviewermeasurepoints.h"
#include "iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.h"
#include "iqvtkconstrainedsketcheditor.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTextEdit>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>

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
#include "vtkCellPicker.h"
#include "vtkImageReader2Factory.h"
#include "vtkImageReader2.h"

#include <vtkCubeSource.h>
#include "vtkImageData.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkPointData.h"
#include "vtkPropPicker.h"
#include "vtkPointSource.h"
#include "vtkPointPicker.h"
#include "vtkOpenGLPolyDataMapper.h"

#include "ivtkoccshape.h"
#include "iqpickinteractorstyle.h"
#include "iqvtkviewwidgetinsertids.h"


#include "base/vtkrendering.h"
#include "iqvtkkeepfixedsizecallback.h"
#include "base/cppextensions.h"

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2); //if render backen is OpenGL2, it should changes to vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);


using namespace insight;

uint
IQVTKCADModel3DViewer::QPersistentModelIndexHash::operator()
( const QPersistentModelIndex& idx ) const
{
    return qHash(idx);
}





void IQVTKCADModel3DViewer::recomputeSceneBounds() const
{
    double
        xmin=DBL_MAX, xmax=-DBL_MAX,
        ymin=DBL_MAX, ymax=-DBL_MIN,
        zmin=DBL_MAX, zmax=-DBL_MAX;

    bool some=false;
    for (const auto& disp: displayedData_)
    {
        if (boost::get<insight::cad::FeaturePtr>(&disp.second.ce_))
        {
            for (const auto& act: disp.second.actors_)
            {
                if (const auto* b = act->GetBounds())
                {
                    some=true;
                    xmin=std::min(xmin, b[0]);
                    xmax=std::max(xmax, b[1]);

                    ymin=std::min(ymin, b[2]);
                    ymax=std::max(ymax, b[3]);

                    zmin=std::min(zmin, b[4]);
                    zmax=std::max(zmax, b[5]);
                }
            }
        }
    }
    if (some)
    {
        sceneBounds_.xmin=xmin;
        sceneBounds_.xmax=xmax;
        sceneBounds_.ymin=ymin;
        sceneBounds_.ymax=ymax;
        sceneBounds_.zmin=zmin;
        sceneBounds_.zmax=zmax;
    }
}


std::weak_ptr<IQVTKViewerState>
IQVTKCADModel3DViewer::highlightActor(vtkProp *prop)
{
    std::shared_ptr<IQVTKViewerState> actorHighlight;
    vtkPolyDataMapper* pdm=nullptr;
    if (auto actor = vtkActor::SafeDownCast(prop))
    {
        if (auto* pdm = vtkPolyDataMapper::SafeDownCast(actor->GetMapper()))
        {
            if (pdm->GetInput()->GetNumberOfPolys()>0)
            {
                actorHighlight.reset(
                    new SilhouetteHighlighter(
                        *this, pdm
                    ));
            }
            else if (pdm->GetInput()->GetNumberOfLines()>0
                     || pdm->GetInput()->GetNumberOfStrips()>0)
            {
                actorHighlight.reset(
                    new LinewidthHighlighter(
                        *this, actor
                    ));
            }
            else if (pdm->GetInput()->GetNumberOfPoints()>0)
            {
                actorHighlight.reset(
                    new PointSizeHighlighter(
                        *this, actor
                    ));
            }
        }
#warning need highlighting for 2D actors as well
    }

    highlightedActors_.insert(actorHighlight);
    return actorHighlight;
}

IQVTKCADModel3DViewer::HighlightingHandleSet
IQVTKCADModel3DViewer::highlightActors(std::set<vtkProp *> actor)
{
    HighlightingHandleSet ret;
    for (auto& a: actor)
    {
        ret.insert(highlightActor(a));
    }
    return ret;
}

void IQVTKCADModel3DViewer::unhighlightActor(
    HighlightingHandle highlighter)
{
    auto i = highlightedActors_.find(highlighter.lock());
    if (i!=highlightedActors_.end())
    {
        highlightedActors_.erase(i);
    }
}

void IQVTKCADModel3DViewer::unhighlightActors(
    HighlightingHandleSet highlighters)
{
    for (auto& hl: highlighters)
    {
        unhighlightActor(hl);
    }
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
            recomputeSceneBounds();
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

            plane->SetCenter(toArray(vec3(pl.Location())));
            plane->SetNormal(toArray(vec3(pl.Direction())));


            auto ks = vtkSmartPointer<IQVTKKeepFixedSize>::New();
            ks->SetViewer(this);
            ks->SetPRef(vec3(pl.Location()));
            ks->SetInputConnection(plane->GetOutputPort());
            this->ren_->AddObserver(vtkCommand::AnyEvent, ks);

            actor->GetMapper()->SetInputConnection(ks->GetOutputPort());
            actor->GetProperty()->SetOpacity(0.33);
        }
        else if (datum->providesAxisReference())
        {
            auto ax = datum->axis();
            auto p1 = vec3(ax.Location());
            auto dir = normalized(vec3(ax.Direction()));

            arma::mat from = p1 - 0.5*dir;
            arma::mat to = p1 + 0.5*dir;

            auto l = vtkSmartPointer<vtkLineSource>::New();

            l->SetPoint1( toArray(from) );
            l->SetPoint2( toArray(to) );

            auto ks = vtkSmartPointer<IQVTKKeepFixedSize>::New();
            ks->SetViewer(this);
            ks->SetPRef(p1);
            ks->SetInputConnection(l->GetOutputPort());
            this->ren_->AddObserver(vtkCommand::AnyEvent, ks);

            actor->GetMapper()->SetInputConnection(ks->GetOutputPort());
            auto prop=actor->GetProperty();
            prop->SetLineWidth(2);
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
        auto shape = vtkSmartPointer<ivtkOCCShape>::New();
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

        vtkSmartPointer<vtkProp> actor;

        if (auto ids = vtkImageData::SafeDownCast(ds) )
        {
            auto act = vtkSmartPointer<vtkImageActor>::New();
            auto mapper = act->GetMapper();
            mapper->SetInputData(ids);
//            ren_->AddActor(act);
            actor = act;
        }
        else if (auto pds = vtkDataSet::SafeDownCast(ds) )
        {
            auto act = vtkSmartPointer<vtkActor>::New();
            act->SetMapper( vtkSmartPointer<vtkDataSetMapper>::New() );
            auto mapper=act->GetMapper();

            mapper->SetInputDataObject(pds);
//            ren_->AddActor(act);
            actor = act;
        }
        else
        {
            auto act = vtkSmartPointer<vtkActor>::New();
            act->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
            act->GetMapper()->SetInputDataObject(ds);
//            ren_->AddActor(act);
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
        prop->SetPointSize(6);
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
        {
            ren_->AddActor(a);
        }
        displayedData_[pidx]={lbl, entity, actors};
        recomputeSceneBounds();
        resetDisplayProps(pidx);
        viewState_.resetCameraIfAllow();
    }
    scheduleRedraw();
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
        QModelIndex idx(pidx);
        auto visible = idx.siblingAtColumn(IQCADItemModel::visibilityCol)
                .data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;

        for (const auto& actor: displayedData_[pidx].actors_)
        {
            if (auto act = vtkActor::SafeDownCast(actor))
            {
                act->SetVisibility(visible);
            }
            else if (auto act = vtkActor2D::SafeDownCast(actor))
            {
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

void IQVTKCADModel3DViewer::closeEvent(QCloseEvent *ev)
{
    if (currentUserActivity_)
    {
        auto answer= QMessageBox::question(this,
            "User activity in progress",
            "There is a user activity in progress!\nAny data will be lost.\nReally proceed?");
        if (answer!=QMessageBox::StandardButton::Yes)
        {
            ev->ignore();
            return;
        }
    }

    ev->accept();
}


IQVTKCADModel3DViewer::BackgroundImage::BackgroundImage(
        const boost::filesystem::path& imageFile,
        IQVTKCADModel3DViewer& v )
    : IQVTKViewerState(v)
{
    insight::assertion(
                boost::filesystem::exists(imageFile),
                "image file has to exist!" );

    auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkSmartPointer<vtkImageReader2> imageReader;
    auto ir=readerFactory->CreateImageReader2(imageFile.string().c_str());

    insight::assertion(
                ir!=nullptr,
                "could not create reader for file "+imageFile.string() );

    imageReader.TakeReference(ir);
    imageReader->SetFileName(imageFile.string().c_str());
    imageReader->Update();
    auto imageData = imageReader->GetOutput();
    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData);
    viewer().backgroundRen_->AddActor(imageActor_);

    viewer().renWin()->Render();

    double origin[3];
    double spacing[3];
    int extent[6];
    imageData->GetOrigin(origin);
    imageData->GetSpacing(spacing);
    imageData->GetExtent(extent);

    vtkCamera* camera = viewer().backgroundRen_->GetActiveCamera();
    camera->ParallelProjectionOn();

    double xc = origin[0] + 0.5 * (extent[0] + extent[1]) * spacing[0];
    double yc = origin[1] + 0.5 * (extent[2] + extent[3]) * spacing[1];
    // double xd = (extent[1] - extent[0] + 1)*spacing[0];
    double yd = (extent[3] - extent[2] + 1) * spacing[1];
    double d = camera->GetDistance();
    camera->SetParallelScale(0.5 * yd);
    camera->SetFocalPoint(xc, yc, 0.0);
    camera->SetPosition(xc, yc, d);

    viewer().scheduleRedraw();
}

IQVTKCADModel3DViewer::BackgroundImage::~BackgroundImage()
{
    viewer().backgroundRen_->RemoveActor( imageActor_ );
    viewer().scheduleRedraw();
}


void IQVTKCADModel3DViewer::setBackgroundImage(const boost::filesystem::path &imageFile)
{
    backgroundImage_.reset();

    if (!imageFile.empty())
    {
        backgroundImage_.reset(new BackgroundImage(imageFile, *this));
    }
}




void IQVTKCADModel3DViewer::onMeasureDistance()
{
    launchUserActivity(
        std::make_shared<IQVTKCADModel3DViewerMeasurePoints>(*this),
        true );
}




void IQVTKCADModel3DViewer::onSelectPoints()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertPointIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchUserActivity(md, true);
}




void IQVTKCADModel3DViewer::onSelectEdges()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertEdgeIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchUserActivity(md, true);
}




void IQVTKCADModel3DViewer::onSelectFaces()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertFaceIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchUserActivity(md, true);
}




void IQVTKCADModel3DViewer::onSelectSolids()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertSolidIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchUserActivity(md, true);
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
                        nullptr, idisp->first, *this );
        }
        else
        {
            // not in display, add temporarily
            auto actor = createActor(feat);
            highlightedItem_ = std::make_shared<HighlightItem>(
                        std::shared_ptr<DisplayedEntity>(
                            new DisplayedEntity{name, item, {actor}} ),
                        QPersistentModelIndex(),
                        *this );
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


void IQVTKCADModel3DViewer::scheduleRedraw(int millisec)
{
    redrawTimer_.start(millisec);
}


void IQVTKCADModel3DViewer::undoHighlightItem()
{
    highlightedItem_.reset();
}


vtkRenderWindow* IQVTKCADModel3DViewer::renWin()
{
    return vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
    renderWindow
#else
    GetRenderWindow
#endif
    ();
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
    setCentralWidget(&vtkWidget_);

    setMouseTracking( true );
    setFocusPolicy(Qt::StrongFocus);

    backgroundRen_ = vtkSmartPointer<vtkRenderer>::New();
    backgroundRen_->SetLayer(0);
    backgroundRen_->InteractiveOff();
    ren_ = vtkSmartPointer<vtkRenderer>::New();
    ren_->SetLayer(1);

    renWin()->SetNumberOfLayers(2);
    renWin()->AddRenderer(backgroundRen_);
    renWin()->AddRenderer(ren_);

    backgroundRen_->SetBackground(1., 1., 1.);
//    ren_->SetBackground(1., 1., 1.);


    auto btntb = this->addToolBar("View setup");

    btntb->addAction(
                QPixmap(":/icons/icon_fit_all.svg"), "Fit all",
                this, &IQCADModel3DViewer::fitAll);

    btntb->addAction(
                QPixmap(":/icons/icon_plusx.svg"), "+X",
                this, &IQCADModel3DViewer::viewFront );

    btntb->addAction(
                QPixmap(":/icons/icon_minusx.svg"), "-X",
                this, &IQCADModel3DViewer::viewBack );

    btntb->addAction(
                QPixmap(":/icons/icon_plusy.svg"), "+Y",
                this, &IQCADModel3DViewer::viewRight );

    btntb->addAction(
                QPixmap(":/icons/icon_minusy.svg"), "-Y",
                this, &IQCADModel3DViewer::viewLeft );

    btntb->addAction(
                QPixmap(":/icons/icon_plusz.svg"), "+Z",
                this, &IQCADModel3DViewer::viewBottom );

    btntb->addAction(
                QPixmap(":/icons/icon_minusz.svg"), "-Z",
                this, &IQCADModel3DViewer::viewTop );

    btntb->addAction(
                "BG",
                [this]() {
                    auto fn = QFileDialog::getOpenFileName(this, "Select background image file");
                    if (!fn.isEmpty())
                    {
                        setBackgroundImage(fn.toStdString());
                    }
                }
    );
    btntb->addAction(
                "CLBG",
                [this]() {
                    setBackgroundImage(boost::filesystem::path());
                }
    );
    btntb->addAction(
        "PP/CP",
        [this]() {
            ren_->GetActiveCamera()->SetParallelProjection(
                !ren_->GetActiveCamera()->GetParallelProjection()
                );
        }
        );

    auto axes = vtkSmartPointer<vtkAxesActor>::New();

    // Call vtkRenderWindowInteractor in orientation marker widgt
    auto widget = vtkOrientationMarkerWidget::New();
    widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes );
    widget->SetInteractor( renWin()->GetInteractor() );
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
            renWin()->Render();
        }
    );
}



IQVTKCADModel3DViewer::~IQVTKCADModel3DViewer()
{
    clipping_.reset();
    highlightedItem_.reset();
    highlightedActors_.clear();
    backgroundImage_.reset();
//    displayedSketch_.reset();
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
    return vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
        interactor
#else
        GetInteractor
#endif
        ();
}




vtkRenderer* IQVTKCADModel3DViewer::renderer()
{
    return ren_;
}

const vtkRenderer *IQVTKCADModel3DViewer::renderer() const
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
            bool anythingVisible=false;
            for (const auto& act: disp.second.actors_)
                anythingVisible = anythingVisible || act->GetVisibility();

            if (anythingVisible)
                activateSelection(*featPtr, subshapeType);
        }
    }
}



void IQVTKCADModel3DViewer::deactivateSubshapeSelectionAll()
{
    currentSubshapeSelection_.reset();
}



vtkProp *IQVTKCADModel3DViewer::findActorUnderCursorAt(const QPoint& clickPos) const
{
    auto p = widgetCoordsToVTK(clickPos);

    // Pick from this location.
//    insight::dbg()
//            << "click pos = "
//            << clickPos.x()<<" "<<clickPos.y()
//            << " ("<<p.x()<<" "<<p.y()<<")"
//            << std::endl;

//    auto picker = vtkSmartPointer<vtkPointPicker>::New();
//    auto picker = vtkSmartPointer<vtkCellPicker>::New();
    auto picker2d = vtkSmartPointer<vtkPropPicker>::New();
    picker2d->Pick(p.x(), p.y(), 0, ren_);

    auto act2 = picker2d->GetActor2D();
    if (act2)
    {
        return act2;
    }
    else
    {
        auto picker3d = vtkSmartPointer<vtkPicker>::New();
        picker3d->SetTolerance(1e-4);
        picker3d->Pick(p.x(), p.y(), 0, ren_);
        auto pi = picker3d->GetActors();
        std::cout<<"# under cursors = "<<pi->GetNumberOfItems()<<std::endl;
        if (pi->GetNumberOfItems()>0)
        {
            pi->InitTraversal();
            return pi->GetNextProp();
        }
    }

    return nullptr;
}



std::vector<vtkProp *> IQVTKCADModel3DViewer::findAllActorsUnderCursorAt(const QPoint &clickPos) const
{
    std::vector<vtkProp *> aa;

    auto p = widgetCoordsToVTK(clickPos);

    auto picker2d = vtkSmartPointer<vtkPropPicker>::New();
    picker2d->Pick(p.x(), p.y(), 0, ren_);
    if (auto act2 = picker2d->GetActor2D())
    {
        aa.push_back(act2);
    }

    // take a closer look with other pick engine
    // which can detect multiple overlapping actors
    auto picker3d = vtkSmartPointer<vtkPicker>::New();
//    picker3d->SetTolerance(1e-4);
    picker3d->Pick(p.x(), p.y(), 0, ren_);
    auto pi = picker3d->GetActors();
    std::cout<<"# under cursors = "<<pi->GetNumberOfItems()<<std::endl;
    pi->InitTraversal();
    while (auto *p= pi->GetNextProp())
    {
        aa.push_back(p);
    }

    return aa;
}


IQVTKCADModel3DViewer::ItemAtCursor
IQVTKCADModel3DViewer::findUnderCursorAt(const QPoint& clickPos) const
{
    if (auto* pickedActor = findActorUnderCursorAt(clickPos))
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




arma::mat
IQVTKCADModel3DViewer::pointInPlane2D(const gp_Ax3& plane, const arma::mat &p3) const
{
    insight::assertion(
        p3.n_elem==3,
        "expected 3D vector, got %d", p3.n_elem );

    gp_Trsf pl;
    pl.SetTransformation(plane); // from global to plane
    auto pp = toVec<gp_Pnt>(p3).Transformed(pl);
    insight::assertion(
        fabs(pp.Z())<SMALL,
        "point (%g, %g, %g) not in plane with p=(%g, %g, %g) and n=(%g, %g, %g)!\n"
        "(local coordinates = (%g, %g, %g))",
        p3(0), p3(1), p3(2),
        plane.Location().X(), plane.Location().Y(), plane.Location().Z(),
        plane.Direction().X(), plane.Direction().Y(), plane.Direction().Z(),
        pp.X(), pp.Y(), pp.Z() );
    return vec2( pp.X(), pp.Y() );
}




arma::mat IQVTKCADModel3DViewer::pointInPlane3D(const gp_Ax3& plane, const arma::mat &pip2d) const
{
    insight::assertion(
        pip2d.n_elem==2,
        "expected 2D vector, got %d", pip2d.n_elem );

    gp_Trsf pl;
    pl.SetTransformation(plane); // from global to plane
    gp_Pnt p2(pip2d(0), pip2d(1), 0);
    auto pp = p2.Transformed(pl.Inverted());
    return vec3(pp);
}




arma::mat
IQVTKCADModel3DViewer::pointInPlane3D(
    const gp_Ax3& plane,
    const QPoint &screenPos ) const
{
    auto *renderer = const_cast<IQVTKCADModel3DViewer*>(this)->renderer();
    auto v = widgetCoordsToVTK(screenPos);

    arma::mat p0=vec3(plane.Location());
    arma::mat n=vec3(plane.Direction());

    arma::mat l0=vec3Zero();
    renderer->SetDisplayPoint(v.x(), v.y(), 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(l0.memptr());

    arma::mat camPos, camFocal;
    camPos=camFocal=vec3Zero();
    renderer->GetActiveCamera()->GetPosition(camPos.memptr());
    renderer->GetActiveCamera()->GetFocalPoint(camFocal.memptr());
    arma::mat l = normalized(camFocal-camPos);

    double nom=arma::dot((p0-l0),n);
    insight::assertion(
        fabs(nom)>SMALL,
        "no single intersection" );

    double denom=arma::dot(l,n);
    insight::assertion(
        fabs(denom)>SMALL,
        "no intersection" );

    double d=nom/denom;

    return l0+l*d;
}




arma::mat
IQVTKCADModel3DViewer::pointInPlane2D(const gp_Ax3& plane, const QPoint &screenPos) const
{
    return pointInPlane2D(plane, pointInPlane3D(plane, screenPos));
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


void IQVTKCADModel3DViewer::doSketchOnPlane(insight::cad::DatumPtr plane)
{

    // free sketch name
    auto feats = cadmodel()->modelsteps();
    auto lbl = insight::findUnusedLabel(
                feats.begin(), feats.end(),
                std::string("sketch"),
                [](decltype(feats)::const_iterator i)
                 -> std::string
                    {
                      return i->first;
                    }
                );

    auto name = QInputDialog::getText(
                this, "Create sketch", "Name of the sketch",
                QLineEdit::Normal, QString::fromStdString(lbl)
                );

    if (!name.isEmpty())
    {
        auto sk = std::dynamic_pointer_cast<insight::cad::ConstrainedSketch>(
                    insight::cad::ConstrainedSketch::create(plane));
        editSketch(sk, insight::ParameterSet(),
                   [](const insight::ParameterSet&, vtkProperty* actprops)
                   {
                       actprops->SetColor(1,0,0);
                       actprops->SetLineWidth(2);
                   },
                   [this,name,sk]() {
                        cadmodel()->addModelstep(name.toStdString(), sk);
                        cadmodel()->setStaticModelStep(name.toStdString(), true);
                    }
            );
    }
}



void IQVTKCADModel3DViewer::editSketch(
    insight::cad::ConstrainedSketchPtr psk,
    const insight::ParameterSet& defaultGeometryParameters,
    SetSketchEntityAppearanceCallback saac,
    SketchCompletionCallback scc )
{
    if (!currentUserActivity_)
    {
        auto ske = std::make_unique<IQVTKConstrainedSketchEditor>(
            *this,
            psk,
            defaultGeometryParameters,
            saac );


        connect(ske.get(), &IQVTKConstrainedSketchEditor::finished, ske.get(),
                [this,psk,scc]()
                {
                    if (scc) scc();
                    currentUserActivity_.reset();
                }
        );

        currentUserActivity_=std::move(ske);
    }
}





QSize IQVTKCADModel3DViewer::sizeHint() const
{
    return QSize(1024,768);
}

static const double DevicePixelRatioTolerance = 1e-5;

QPointF IQVTKCADModel3DViewer::widgetCoordsToVTK(const QPoint &widgetCoords) const
{
    auto pw = vtkWidget_.mapFromParent(widgetCoords);
    double DevicePixelRatio=vtkWidget_.devicePixelRatioF();
    QPoint p(
        static_cast<int>(pw.x() * DevicePixelRatio + DevicePixelRatioTolerance),
        static_cast<int>(pw.y() * DevicePixelRatio + DevicePixelRatioTolerance)
        );
    return QPointF(
                p.x(),
                vtkWidget_.size().height()-p.y()-1
        );
}



bool IQVTKCADModel3DViewer::launchUserActivity(
    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr activity, bool force )
{
    if (force)
    {
        currentUserActivity_.reset();
    }

    if (!currentUserActivity_)
    {
        currentUserActivity_=activity;
        return true;
    }
    else
        return false;
}

const IQVTKCADModel3DViewer::Bounds &IQVTKCADModel3DViewer::sceneBounds() const
{
    return sceneBounds_;
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


void IQVTKCADModel3DViewer::mouseDoubleClickEvent(QMouseEvent *e)
{
    if ( e->button() & Qt::LeftButton )
    {
        bool ret=navigationManager_->onLeftButtonDoubleClick( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onLeftButtonDoubleClick( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onLeftButtonDoubleClick( e->modifiers(), e->pos() );
    }

    if (currentUserActivity_ && currentUserActivity_->finished())
        currentUserActivity_.reset();

}

void IQVTKCADModel3DViewer::mousePressEvent   ( QMouseEvent* e )
{
    if ( e->button() & Qt::LeftButton )
    {
        bool ret=navigationManager_->onLeftButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onLeftButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onLeftButtonDown( e->modifiers(), e->pos() );
    }
    else if ( e->button() & Qt::RightButton )
    {
        bool ret=navigationManager_->onRightButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onRightButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onRightButtonDown( e->modifiers(), e->pos() );
    }
    else if ( e->button() & Qt::MidButton )
    {
        bool ret=navigationManager_->onMiddleButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onMiddleButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onMiddleButtonDown( e->modifiers(), e->pos() );
    }

    if (currentUserActivity_ && currentUserActivity_->finished())
        currentUserActivity_.reset();


}




void IQVTKCADModel3DViewer::mouseReleaseEvent ( QMouseEvent* e )
{
    if ( e->button() & Qt::LeftButton )
    {
        bool ret=navigationManager_->onLeftButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onLeftButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onLeftButtonUp( e->modifiers(), e->pos() );
    }
    else if ( e->button() & Qt::RightButton )
    {
        bool ret=navigationManager_->onRightButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onRightButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onRightButtonUp( e->modifiers(), e->pos() );
        if (!ret)
        {
            auto pickedActor = findActorUnderCursorAt(e->pos());

            if (pickedActor != nullptr)
            {
                auto i = std::find_if(
                            displayedData_.begin(),
                            displayedData_.end(),
                            [pickedActor](const DisplayedData::value_type& e)
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
        bool ret=navigationManager_->onMiddleButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentNavigationAction_)
            ret=currentNavigationAction_->onMiddleButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentUserActivity_)
            ret=currentUserActivity_->onMiddleButtonUp( e->modifiers(), e->pos() );
    }

    if (currentUserActivity_ && currentUserActivity_->finished())
        currentUserActivity_.reset();
}




void IQVTKCADModel3DViewer::mouseMoveEvent    ( QMouseEvent* e )
{

//    if (auto* ac = findActorUnderCursorAt(e->pos()))
//    {
//        highlightActor(ac);
//    }
//    else
//    {
//        actorHighlight_.reset();
//    }

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

    bool ret=navigationManager_->onKeyPress(e->modifiers(), e->key());

    if (!ret && currentNavigationAction_)
      ret=currentNavigationAction_->onKeyPress(e->modifiers(), e->key());

    if (!ret && currentUserActivity_)
      ret=currentUserActivity_->onKeyPress(e->modifiers(), e->key());

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();

    if (!ret) QWidget::keyPressEvent(e);
}




void IQVTKCADModel3DViewer::keyReleaseEvent   ( QKeyEvent* e )
{
    insight::dbg()<<"key release event"<<std::endl;

    bool ret=navigationManager_->onKeyRelease(e->modifiers(), e->key());

    if (!ret && currentNavigationAction_)
      ret=currentNavigationAction_->onKeyRelease(e->modifiers(), e->key());

    if (!ret && currentUserActivity_)
      ret=currentUserActivity_->onKeyRelease(e->modifiers(), e->key());

    if (currentUserActivity_ && currentUserActivity_->finished())
      currentUserActivity_.reset();

    if (!ret) QWidget::keyReleaseEvent(e);
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

IQVTKCADModel3DViewer::Bounds::Bounds()
    : xmin(0), xmax(0), ymin(0), ymax(0), zmin(0), zmax(0)
{}
