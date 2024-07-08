#include "iqvtkcadmodel3dviewer.h"
#include "constrainedsketch.h"
#include "iqcaditemmodel.h"
#include "iscadmetatyperegistrator.h"
#include "postprocactionvisualizer.h"
#include "datum.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewermeasurepoints.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewermeasurediameter.h"
#include "iqcadmodel3dviewer/iqvtkcadmodel3dviewerrotation.h"
#include "iqcadmodel3dviewer/iqvtkcadmodel3dviewerpanning.h"
#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerdrawline.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/orientbackgroundimage.h"
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
#include <QToolButton>
#include <QStatusBar>

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
#include "vtkCaptionActor2D.h"
#include "vtkInteractorStyleUser.h"
#include "vtkProp3DCollection.h"

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
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkviewwidgetinsertids.h"


#include "base/vtkrendering.h"
#include "base/rapidxml.h"
#include "iqvtkkeepfixedsizecallback.h"
#include "base/cppextensions.h"
#include "iqvtkviewer.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkselectcadentity.h"

#include <QDebug>

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2); //if render backen is OpenGL2, it should changes to vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);



using namespace insight;



MyVTKWidget::MyVTKWidget(QWidget *parent)
    : VTKWidget(parent)
{}



void MyVTKWidget::leaveEvent(QEvent *event)
{
    dbg(3)<<"leave event"<<std::endl;

    VTKWidget::leaveEvent(event);
    Q_EMIT mouseLeavesViewer();
}


void MyVTKWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
    e->ignore();
}

void MyVTKWidget::mousePressEvent(QMouseEvent* e)
{
    e->ignore();
}

void MyVTKWidget::mouseReleaseEvent(QMouseEvent* e)
{
    e->ignore();
}

void MyVTKWidget::mouseMoveEvent(QMouseEvent* e)
{
    e->ignore();
}

void MyVTKWidget::wheelEvent(QWheelEvent* e)
{
    e->ignore();
}

void MyVTKWidget::keyPressEvent(QKeyEvent* e)
{
    e->ignore();
}

void MyVTKWidget::keyReleaseEvent(QKeyEvent* e)
{
    e->ignore();
}



IQVTKCADModel3DViewer &BackgroundImage::viewer()
{
    return dynamic_cast<IQVTKCADModel3DViewer&>(*parent());
}

BackgroundImage::BackgroundImage(
    const boost::filesystem::path& imageFile,
    IQVTKCADModel3DViewer& v )

  : QObject(&v),
    imageFileName_(imageFile),
    label_(QString::fromStdString(imageFile.filename().string()))
{
    insight::assertion(
        boost::filesystem::exists(imageFileName_),
        "image file has to exist!" );

    auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkSmartPointer<vtkImageReader2> imageReader;
    auto ir=readerFactory->CreateImageReader2(imageFileName_.string().c_str());

    insight::assertion(
        ir!=nullptr,
        "could not create reader for file "+imageFileName_.string() );

    imageReader.TakeReference(ir);
    imageReader->SetFileName(imageFileName_.string().c_str());
    imageReader->Update();
    auto imageData = imageReader->GetOutput();
    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData);

    QMessageBox questionBox;
    questionBox.setWindowTitle("Image display");
    questionBox.setText("Shall the image be a static background or dynamic rotatable/zoomable?");
    auto* pButtonStatic = questionBox.addButton("Static", QMessageBox::YesRole);
    auto* pButtonDynamic = questionBox.addButton("Dynamic", QMessageBox::NoRole);
    auto* pButtonCancel = questionBox.addButton(QMessageBox::StandardButton::Cancel);

    questionBox.exec();

    auto answer = questionBox.clickedButton();

    if (answer==pButtonStatic)
    {
        usedRenderer_ = viewer().backgroundRen_;
        usedRenderer_->AddActor(imageActor_);

        viewer().renWin()->Render();

        IQImageReferencePointSelectorWindow::setupCameraForImage(
            imageData, usedRenderer_->GetActiveCamera());

        viewer().scheduleRedraw();
    }
    else if (answer==pButtonDynamic)
    {
        imageActor_->SetScale( 1 );
        imageActor_->SetPosition( 0, 0, 0 );

        usedRenderer_=viewer().renderer();
        usedRenderer_->AddActor(imageActor_);
        viewer().scheduleRedraw();

        auto obia=std::make_shared<IQVTKOrientBackgroundImage>(viewer(), imageActor_);
        obia->orientationSelected.connect(
            [this](OrientationSpec os)
            {
                arma::mat D=os.xy2_-os.xy1_;
                arma::mat d=os.p2_-os.pCtr_;
                double scale =
                    arma::norm(D,2)
                    /
                    arma::norm(d,2);

                double ang=atan2(D[1], D[0]);
                double ang2=atan2(d[1], d[0]);
                imageActor_->SetOrientation(0, 0, (ang-ang2)*180./M_PI);

                arma::mat R = rotMatrix(ang-ang2);

                imageActor_->SetScale( scale );

                arma::mat newPosition( -scale*R*os.pCtr_ +os.xy1_ );
                imageActor_->SetPosition( newPosition.memptr() );

                imageActor_->SetOpacity(0.8); // can't pick if opacity is lower than 1

                viewer().scheduleRedraw();
            }
            );

        viewer().launchAction(
            obia,
            true );
    }
}

BackgroundImage::BackgroundImage(
    const rapidxml::xml_node<>& node,
    IQVTKCADModel3DViewer& v )

  : QObject(&v)
{
    label_= QString(
        node.first_attribute("label")->value() );

    imageFileName_= boost::filesystem::path(
        node.first_attribute("imageFileName")->value() );

    insight::assertion(
        boost::filesystem::exists(imageFileName_),
        "image file has to exist!" );

    auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkSmartPointer<vtkImageReader2> imageReader;
    auto ir=readerFactory->CreateImageReader2(imageFileName_.string().c_str());

    insight::assertion(
        ir!=nullptr,
        "could not create reader for file "+imageFileName_.string() );

    imageReader.TakeReference(ir);
    imageReader->SetFileName(imageFileName_.string().c_str());
    imageReader->Update();
    auto imageData = imageReader->GetOutput();
    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData);

    double rotationAngle = toNumber<double>(
        node.first_attribute("rotationAngle")->value() );
    imageActor_->SetOrientation(0, 0, rotationAngle);

    double scale = toNumber<double>(
        node.first_attribute("scale")->value() );
    imageActor_->SetScale( scale );

    arma::mat position;
    stringToValue(
        node.first_attribute("position")->value(),
        position );
    imageActor_->SetPosition( position.memptr() );

    imageActor_->SetOpacity(0.8); // can't pick if opacity is lower than 1

    usedRenderer_=viewer().renderer();
    usedRenderer_->AddActor(imageActor_);
    viewer().scheduleRedraw();
}




BackgroundImage::~BackgroundImage()
{
    usedRenderer_->RemoveActor( imageActor_ );
    viewer().scheduleRedraw();
}




QString BackgroundImage::label() const
{
    return label_;
}




void BackgroundImage::write(
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node ) const
{
    insight::appendAttribute(
        doc, node,
        "label", label_.toStdString());

    insight::appendAttribute(
        doc, node, "imageFileName",
        imageFileName_.string());

    insight::appendAttribute(
        doc, node,
        "rotationAngle",
        boost::lexical_cast<std::string>(imageActor_->GetOrientation()[2]) );

    insight::appendAttribute(
        doc, node,
        "scale",
        boost::lexical_cast<std::string>(imageActor_->GetScale()[0]) );


    arma::mat position = vec3Zero();
    imageActor_->GetPosition(position.memptr());
    insight::appendAttribute(
        doc, node,
        "position", valueToString(position) );
}




void BackgroundImage::toggleVisibility(bool show)
{
    imageActor_->SetVisibility(show);
    viewer().scheduleRedraw();
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
                if (act->GetVisibility()==true)
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




IQVTKCADModel3DViewer::HighlightingHandle
IQVTKCADModel3DViewer::highlightActor(vtkProp *prop, QColor hicol)
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
                        *this, pdm, hicol
                    ));
            }
            else if (pdm->GetInput()->GetNumberOfLines()>0
                     || pdm->GetInput()->GetNumberOfStrips()>0)
            {
                actorHighlight.reset(
                    new LinewidthHighlighter(
                        *this, actor, hicol
                    ));
            }
            else if (pdm->GetInput()->GetNumberOfPoints()>0)
            {
                actorHighlight.reset(
                    new PointSizeHighlighter(
                        *this, actor, hicol
                    ));
            }
        }
    }
    else if (auto actor2d = vtkCaptionActor2D::SafeDownCast(prop))
    {
        actorHighlight.reset(
            new TextActorHighlighter(
                *this, actor2d, hicol
                ));
    }

    scheduleRedraw();

    return actorHighlight;
}




IQVTKCADModel3DViewer::HighlightingHandleSet
IQVTKCADModel3DViewer::highlightActors(std::set<vtkProp *> actor, QColor hicol)
{
    HighlightingHandleSet ret;
    for (auto& a: actor)
    {
        ret.insert(highlightActor(a, hicol));
    }
    return ret;
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
        if (feat->topologicalProperties().onlyEdges())
        {
            // if shape consists only of edges,
            // create a set of actors for each edge
            // to be able to pick locations inside of
            // a possible edge loop without
            // triggering a selection
            std::vector<vtkSmartPointer<vtkProp> > actors;
            for (TopExp_Explorer ex(feat->shape(),TopAbs_EDGE); ex.More(); ex.Next())
            {
                auto shape = vtkSmartPointer<ivtkOCCShape>::New();
                shape->SetShape( ex.Current() );

                auto actor = vtkSmartPointer<vtkActor>::New();
                actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
                actor->GetMapper()->SetInputConnection(shape->GetOutputPort());

                actors.push_back(actor);
            }
            return actors;
        }
        else
        {
            auto shape = vtkSmartPointer<ivtkOCCShape>::New();
            shape->SetShape( feat->shape() );

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
            actor->GetMapper()->SetInputConnection(shape->GetOutputPort());
            return {actor};
        }

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
            actor = act;
        }
        else if (auto pds = vtkDataSet::SafeDownCast(ds) )
        {
            auto act = vtkSmartPointer<vtkActor>::New();
            act->SetMapper( vtkSmartPointer<vtkDataSetMapper>::New() );
            auto mapper=act->GetMapper();

            mapper->SetInputDataObject(pds);
            actor = act;
        }
        else
        {
            auto act = vtkSmartPointer<vtkActor>::New();
            act->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
            act->GetMapper()->SetInputDataObject(ds);
            actor = act;
        }

        return {actor};
    }

    return {};
}


std::vector<vtkSmartPointer<vtkProp> > IQVTKCADModel3DViewer::findActorsOf(const QPersistentModelIndex& pidx) const
{
    auto i = displayedData_.find(pidx);
    if (i!=displayedData_.end())
    {
        return i->second.actors_;
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
            if (clipping_) clipping_->addToActor(a);
        }
        displayedData_[pidx]={lbl, entity, actors};
        resetVisibility(pidx);
        recomputeSceneBounds();
        resetDisplayProps(pidx);
        viewState_.resetCameraIfAllow();
    }
    scheduleRedraw();
}



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
                if (auto act = vtkActor::SafeDownCast(actor))
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
                        if (pointCell==insight::OnPoint)
                        {
                            if (auto *arr = pds->GetPointData()->GetArray(0))
                            {
                                fieldName = arr->GetName();
                            }
                        }
                        else if (pointCell==insight::OnCell)
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
                        if (pointCell==insight::OnPoint)
                        {
                            pds->GetPointData()->GetArray(fieldName.c_str())->GetRange(mima, component);
                        }
                        else if (pointCell==insight::OnCell)
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
    if (!isDefaultAction())
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





void IQVTKCADModel3DViewer::onMeasureDistance()
{
    launchAction(
        std::make_shared<IQVTKCADModel3DViewerMeasurePoints>(*this),
        true );
}




void IQVTKCADModel3DViewer::onMeasureDiameter()
{
    launchAction(
        std::make_shared<IQVTKCADModel3DViewerMeasureDiameter>(*this),
        true );
}


void IQVTKCADModel3DViewer::onSelectPoints()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertPointIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchAction(md, true);
}




void IQVTKCADModel3DViewer::onSelectEdges()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertEdgeIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchAction(md, true);
}




void IQVTKCADModel3DViewer::onSelectFaces()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertFaceIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchAction(md, true);
}




void IQVTKCADModel3DViewer::onSelectSolids()
{
    auto md = std::make_shared<IQVTKViewWidgetInsertSolidIDs>(*this);
    connect(md.get(), &ToNotepadEmitter::appendToNotepad,
            this, &IQVTKCADModel3DViewer::appendToNotepad );
    launchAction(md, true);
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

void IQVTKCADModel3DViewer::exposeItem(insight::cad::FeaturePtr feat)
{
    if (feat)
    {
        doExposeItem(feat);
    }
    else
    {
        exposedItem_.reset();
        exposedItem_ = std::make_shared<ExposeItem>(
            nullptr, QModelIndex(), *this );
    }
}


void IQVTKCADModel3DViewer::doExposeItem(CADEntity item)
{
    if (exposedItem_
        && exposedItem_->entity()==item)
        return;

    exposedItem_.reset(); // clear existing highlighting, if any


    if (const auto* fPtr = boost::get<insight::cad::FeaturePtr>(&item))
    {
        auto feat = *fPtr;
        auto name = QString::fromStdString(feat->featureSymbolName());


        DisplayedData::const_iterator idisp;
        findDisplayedItem(item, &idisp);

        if (idisp!=displayedData_.end())
        {
            // already in display
            exposedItem_ = std::make_shared<ExposeItem>(
                        nullptr, idisp->first, *this );
        }
        else
        {
            // not in display, add temporarily
            auto actor = createActor(feat);
            exposedItem_ = std::make_shared<ExposeItem>(
                        std::shared_ptr<DisplayedEntity>(
                            new DisplayedEntity{name, item, {actor}}
                        ),
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


void IQVTKCADModel3DViewer::redrawNow(bool force)
{
    if (redrawRequestedWithinWaitPeriod_ || force)
    {
        renWin()->Render();
        redrawRequestedWithinWaitPeriod_=false;
    }
}


void IQVTKCADModel3DViewer::scheduleRedraw(int freq)
{
    redrawRequestedWithinWaitPeriod_=false;
    if (redrawTimer_.isActive())
    {
        redrawRequestedWithinWaitPeriod_=true;
    }
    else
    {
        redrawNow();
        redrawTimer_.start(1000/freq);
    }
}





void IQVTKCADModel3DViewer::undoExposeItem()
{
    exposedItem_.reset();
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




void IQVTKCADModel3DViewer::connectBackgroundImageCommands(BackgroundImage *bgi)
{
    auto remAct = new QAction("Remove "+bgi->label(), bgi);
    connect(remAct, &QAction::triggered, remAct,
            std::bind(&QList<BackgroundImage*>::removeOne, &backgroundImages_, bgi) );
    connect(remAct, &QAction::triggered, bgi, &QObject::deleteLater);
    clBGBtn->addAction(remAct);

    auto showHideAct = new QAction(bgi->label(), bgi);
    showHideAct->setCheckable(true);
    connect(
        showHideAct, &QAction::toggled,
        bgi, &BackgroundImage::toggleVisibility );
    addBGBtn->addAction(showHideAct);

    showHideAct->setChecked(true);
}




IQVTKCADModel3DViewer::IQVTKCADModel3DViewer(
        QWidget* parent )
    : IQCADModel3DViewer(parent),
      vtkWidget_(this),
      navigationManager_(
        std::make_shared<
          TouchpadNavigationManager<
           IQVTKCADModel3DViewer, IQVTKCADModel3DViewerPanning, IQVTKCADModel3DViewerRotation
          > >(*this) ),
      viewState_(*this),
    defaultSelectionModel_(nullptr),
    customSelectionModel_(nullptr)
{
    setCentralWidget(&vtkWidget_);

    connect(&vtkWidget_, &MyVTKWidget::mouseLeavesViewer, &vtkWidget_,
        [this]()
        {
            navigationManager_->onMouseLeavesViewer();
            if (currentAction_)
            currentAction_->onMouseLeavesViewer( );
        }
    );

    setMouseTracking( true );

    backgroundRen_ = vtkSmartPointer<vtkRenderer>::New();
    backgroundRen_->SetLayer(0);
    backgroundRen_->InteractiveOff();
    ren_ = vtkSmartPointer<vtkRenderer>::New();
    ren_->SetLayer(1);

    renWin()->SetNumberOfLayers(2);
    renWin()->AddRenderer(backgroundRen_);
    renWin()->AddRenderer(ren_);

    backgroundRen_->SetBackground(1., 1., 1.);


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

    auto clBGAction = btntb->addAction("CLBG");
    clBGBtn=
        dynamic_cast<QToolButton*>(btntb->widgetForAction(clBGAction));

    auto addBGAction = btntb->addAction("BG");
    addBGBtn=
        dynamic_cast<QToolButton*>(btntb->widgetForAction(addBGAction));

    connect(addBGAction, &QAction::triggered, addBGAction,
    [this]() {
        auto fn = QFileDialog::getOpenFileName(this, "Select background image file");
        if (!fn.isEmpty())
        {
            auto bgi = new BackgroundImage(fn.toStdString(), *this);
            backgroundImages_.push_back(bgi);

            connectBackgroundImageCommands(bgi);
        }
    }
    );
    connect(clBGAction, &QAction::triggered, clBGAction,
        [this]()
        {
            for (auto bgi: backgroundImages_) delete bgi;
            backgroundImages_.clear();
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
    widget->SetViewport( 0.0, 0.0, 0.15, 0.15 );
    widget->SetEnabled( 1 );
    widget->InteractiveOff();


    renWin()->GetInteractor()->SetInteractorStyle(nullptr);

    ren_->GetActiveCamera()->SetParallelProjection(true);

    ren_->ResetCamera();

    viewState_.store();

    redrawTimer_.setSingleShot(true);
    connect(
        &redrawTimer_, &QTimer::timeout,
        std::bind(&IQVTKCADModel3DViewer::redrawNow, this, false)
    );

    setDefaultAction();
}



IQVTKCADModel3DViewer::~IQVTKCADModel3DViewer()
{
    // delete some stuff so that their destructors
    // still have access to viewer before actual viewer is destroyed
    clipping_.reset();
    exposedItem_.reset();
    navigationManager_.reset();
    currentAction_.reset();
    for(auto bg: backgroundImages_)
    {
        delete bg;
    }
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

    setSelectionModel(new QItemSelectionModel(model));
    defaultSelectionModel_=customSelectionModel_;
    customSelectionModel_=nullptr;
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


//    vtkActor2D *act2=nullptr;
//    {
//        auto picker2d = vtkSmartPointer<vtkPropPicker>::New();
//        picker2d->Pick(p.x(), p.y(), 0, ren_);

//        act2 = picker2d->GetActor2D();
//    }
//    if (act2)
//    {
//        return act2;
//    }
//    else
    {
        auto picker3d = vtkSmartPointer<vtkPicker>::New();
        picker3d->SetTolerance(1e-4);
        picker3d->Pick(p.x(), p.y(), 0, ren_);
        auto pi = picker3d->GetProp3Ds();
//        std::cout<<"# under cursors = "<<pi->GetNumberOfItems()<<std::endl;
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
//    std::vector<double> dist;

    auto p = widgetCoordsToVTK(clickPos);

    {
        auto picker2d = vtkSmartPointer<vtkPropPicker>::New();
        picker2d->Pick(p.x(), p.y(), 0, ren_);
        if (auto act2 = picker2d->GetActor2D())
        {
            aa.push_back(act2);
        }

        const_cast<IQVTKCADModel3DViewer*>(this)
            ->redrawNow(); // flickering bug (black screen) otherwise
    }
    {
        // take a closer look with other pick engine
        // which can detect multiple overlapping actors
        auto picker3d = vtkSmartPointer<vtkPicker>::New();
    //    picker3d->SetTolerance(1e-4);
        picker3d->Pick(p.x(), p.y(), 0, ren_);

        auto pi = picker3d->GetProp3Ds();
        auto pp = picker3d->GetPickedPositions();
        int nsel = pi->GetNumberOfItems();

        auto cp = insight::vec3FromComponents(ren_->GetActiveCamera()->GetPosition());
        std::map<double, vtkProp*> aa3d;
        for (int i=0; i<nsel; ++i)
        {
            auto prop = vtkProp::SafeDownCast(pi->GetItemAsObject(i));
            if (actorsExcludedFromPicking_.count(prop)<1)
            {
                double dist=arma::norm(insight::vec3FromComponents(pp->GetPoint(i))-cp, 2);
                aa3d[dist]=prop;
            }
        }
        std::transform(
            aa3d.begin(), aa3d.end(),
            std::back_inserter(aa),
            [](decltype(aa3d)::value_type& aa3di) { return aa3di.second; }
            );
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


void IQVTKCADModel3DViewer::setSelectionModel(QItemSelectionModel *selmodel)
{
    if (defaultSelectionModel_)
        defaultSelectionModel_->deleteLater();

    customSelectionModel_=selmodel;

//    connect(customSelectionModel_, &QItemSelectionModel::selectionChanged, customSelectionModel_,
//        [this](const QItemSelection &selected, const QItemSelection &deselected)
//        {
//            for (const auto& i: selected.indexes())
//            {
//                auto acts = findActorsOf(i);
//                for (const auto& a: acts)
//                {
//                    highlightActor(a);
//                }
//            }
//            for (const auto& i: deselected.indexes())
//            {
//                auto acts = findActorsOf(i);
//                for (const auto& a: acts)
//                {
//                    unhighlightActor(a);
//                }
//            }
//        }
//    );

    connect(customSelectionModel_, &QItemSelectionModel::currentChanged, customSelectionModel_,
        [this](const QModelIndex &current, const QModelIndex &previous)
        {
            if (current.isValid())
            {
                auto cdd = displayedData_.find(current.siblingAtColumn(0));
                if (cdd!=displayedData_.end())
                {
                    doExposeItem(cdd->second.ce_);
                }
                else
                {
                    exposedItem_.reset();
                }
            }
        }
    );
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
        editSketch(*sk, insight::ParameterSet(),
                   [](const insight::ParameterSet&, vtkProperty* actprops)
                   {
                       actprops->SetColor(1,0,0);
                       actprops->SetLineWidth(2);
                   },
            [this,name](insight::cad::ConstrainedSketchPtr editedSk) {
                        cadmodel()->addModelstep(name.toStdString(), editedSk, false);
                        cadmodel()->setStaticModelStep(name.toStdString(), true);
                    }
            );
    }
}



void IQVTKCADModel3DViewer::editSketch(
    const insight::cad::ConstrainedSketch& psk,
    const insight::ParameterSet& defaultGeometryParameters,
    SetSketchEntityAppearanceCallback saac,
    SketchCompletionCallback onAccept,
    SketchCompletionCallback onCancel )
{
    if (isDefaultAction())
    {
        auto ske = std::make_unique<IQVTKConstrainedSketchEditor>(
            *this,
            psk,
            defaultGeometryParameters,
            saac );

        insight::cad::ConstrainedSketchPtr skePtr=*ske;

        ske->actionIsFinished.connect(
            [onAccept,onCancel,skePtr](bool accepted)
            {
                if (accepted)
                    onAccept(skePtr);
                else
                    onCancel(skePtr);
            }
        );

        launchAction(std::move(ske));
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



void IQVTKCADModel3DViewer::setDefaultAction()
{
    auto selaction = std::make_shared<IQVTKSelectCADEntity>(*this);
    auto selactionPtr = selaction.get();
    launchAction(selaction);
    connect(
        this, &IQVTKCADModel3DViewer::contextMenuClick, selactionPtr,
        [this,selactionPtr](const QPoint& pGlob)
        {
            CADEntity sel;
            bool sfc=false;
            if (selactionPtr->somethingSelected() &&
                selactionPtr->currentSelection().size()==1)
            {
                sel = *selactionPtr->currentSelection().begin();
                sfc=true;
            }
            else if (selactionPtr->hasPreviewedItem() )
            {
                sel = selactionPtr->previewedItem();
                sfc=true;
            }
            if (sfc)
            {
                auto i = std::find_if(
                    displayedData_.begin(),
                    displayedData_.end(),
                    [sel](const DisplayedData::value_type& e)
                        { return e.second.ce_ == sel; }
                );
                if (i!=displayedData_.end())
                {
                    Q_EMIT contextMenuRequested(
                        i->first,
                        pGlob );
                }
            }
        }
    );

}

bool IQVTKCADModel3DViewer::isDefaultAction()
{
    if (currentAction_)
    {
        return typeid(*currentAction_) == typeid(IQVTKSelectCADEntity);
    }
    return false;
}



bool IQVTKCADModel3DViewer::launchAction(
    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr activity, bool force )
{
    if (force)
    {
        currentAction_.reset();
    }

    if (!currentAction_)
    {
        currentAction_=activity;
        currentAction_->actionIsFinished.connect(
            std::bind(&IQVTKCADModel3DViewer::setDefaultAction, this) );        
        currentAction_->userPrompt.connect(
            std::bind(&QStatusBar::showMessage, statusBar(), std::placeholders::_1, 0) );

        currentAction_->start();

        return true;
    }
    else
        return false;
}

const IQVTKCADModel3DViewer::Bounds &IQVTKCADModel3DViewer::sceneBounds() const
{
    return sceneBounds_;
}

const char IQVTKCADModel3DViewer::bgiNodeName[] = "backgroundImage";

void IQVTKCADModel3DViewer::writeViewerState(
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node ) const
{
    auto* camera = ren_->GetActiveCamera();


    for (const auto& bgi: backgroundImages_)
    {
        auto bgin = insight::appendNode(doc, node, bgiNodeName);
        bgi->write(doc, bgin);
    }
}


void IQVTKCADModel3DViewer::restoreViewerState(
    rapidxml::xml_node<>& node )
{
    backgroundImages_.clear();
    for (auto *n=node.first_node(bgiNodeName); n; n=n->next_sibling(bgiNodeName))
    {
        auto *bgi=new BackgroundImage(*n, *this);
        backgroundImages_.append(bgi);
        connectBackgroundImageCommands(bgi);
    }
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

    scheduleRedraw();
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
    IQCADModel3DViewer::mouseDoubleClickEvent(e);

    if ( e->button() & Qt::LeftButton )
    {
        bool ret=navigationManager_->onLeftButtonDoubleClick( e->modifiers(), e->pos() );
      if (!ret && currentAction_)
            ret=currentAction_->onLeftButtonDoubleClick( e->modifiers(), e->pos() );
    }
}

void IQVTKCADModel3DViewer::mousePressEvent   ( QMouseEvent* e )
{
    dbg(3)<<"mouse press"<<std::endl;

    IQCADModel3DViewer::mousePressEvent(e);

    if ( e->button() & Qt::LeftButton )
    {
        bool ret=navigationManager_->onLeftButtonDown( e->modifiers(), e->pos() );
        if (!ret)
            ret = this->onLeftButtonDown(e->modifiers(), e->pos() );
        if (!ret && currentAction_)
            ret=currentAction_->onLeftButtonDown( e->modifiers(), e->pos() );
    }
    else if ( e->button() & Qt::RightButton )
    {
        bool ret=navigationManager_->onRightButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentAction_)
            ret=currentAction_->onRightButtonDown( e->modifiers(), e->pos() );
    }
    else if ( e->button() & Qt::MidButton )
    {
        bool ret=navigationManager_->onMiddleButtonDown( e->modifiers(), e->pos() );
        if (!ret && currentAction_)
            ret=currentAction_->onMiddleButtonDown( e->modifiers(), e->pos() );
    }
}




void IQVTKCADModel3DViewer::mouseReleaseEvent ( QMouseEvent* e )
{
    dbg(3)<<"mouse release"<<std::endl;

    IQCADModel3DViewer::mouseReleaseEvent(e);

    if ( e->button() & Qt::LeftButton )
    {
        bool ret=navigationManager_->onLeftButtonUp( e->modifiers(), e->pos() );
        if (!ret)
            ret = this->onLeftButtonUp(e->modifiers(), e->pos() );
        if (!ret && currentAction_)
            ret=currentAction_->onLeftButtonUp( e->modifiers(), e->pos() );
    }
    else if ( e->button() & Qt::RightButton )
    {
        bool ret=navigationManager_->onRightButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentAction_)
            ret=currentAction_->onRightButtonUp( e->modifiers(), e->pos() );

        if (!ret)
        {
            Q_EMIT contextMenuClick( mapToGlobal(e->pos()) );
        }
    }
    else if ( e->button() & Qt::MidButton )
    {
        bool ret=navigationManager_->onMiddleButtonUp( e->modifiers(), e->pos() );
        if (!ret && currentAction_)
            ret=currentAction_->onMiddleButtonUp( e->modifiers(), e->pos() );
    }
}




void IQVTKCADModel3DViewer::mouseMoveEvent( QMouseEvent* e )
{
    dbg(3)<<"mouse move"<<std::endl;

    IQCADModel3DViewer::mouseMoveEvent(e);

    bool ret=false;
    if (currentAction_)
        if (!ret) ret=currentAction_->onMouseMove( e->buttons(), e->pos(), e->modifiers() );
    if (!ret) navigationManager_->onMouseMove( e->buttons(), e->pos(), e->modifiers() );
}




void IQVTKCADModel3DViewer::wheelEvent( QWheelEvent* e )
{
    dbg(3)<<"wheel"<<std::endl;

    IQCADModel3DViewer::wheelEvent(e);

    bool ret=false;
    if (currentAction_)
        if (!ret) ret=currentAction_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());
    if (!ret) navigationManager_->onMouseWheel(e->angleDelta().x(), e->angleDelta().y());
}




void IQVTKCADModel3DViewer::keyPressEvent( QKeyEvent* e )
{
    dbg(3)<<"key press"<<std::endl;

    IQCADModel3DViewer::keyPressEvent(e);

    if (e->key() == Qt::Key_Escape)
    {
        setDefaultAction();
    }

    bool ret=navigationManager_->onKeyPress(e->modifiers(), e->key());

    if (!ret)
      ret = this->onKeyPress(e->modifiers(), e->key());

    if (!ret && currentAction_)
      ret=currentAction_->onKeyPress(e->modifiers(), e->key());

    if (!ret) QWidget::keyPressEvent(e);
}




void IQVTKCADModel3DViewer::keyReleaseEvent( QKeyEvent* e )
{
    dbg(3)<<"key release"<<std::endl;

    IQCADModel3DViewer::keyReleaseEvent(e);

    bool ret=navigationManager_->onKeyRelease(e->modifiers(), e->key());

    if (!ret && currentAction_)
      ret=currentAction_->onKeyRelease(e->modifiers(), e->key());

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

