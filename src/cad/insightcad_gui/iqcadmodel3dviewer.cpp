#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "iscadmetatyperegistrator.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpacerItem>

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

#include <vtkCubeSource.h>
#include "vtkImageData.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"

#include "ivtkoccshape.h"
#include "iqpickinteractorstyle.h"

#include "datum.h"

#include "base/vtkrendering.h"


#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2); //if render backen is OpenGL2, it should changes to vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);


void IQCADModel3DViewer::remove(const QPersistentModelIndex& pidx)
{
    if (pidx.isValid())
    {
        auto i = displayedData_.find(pidx);
        if (i!=displayedData_.end())
        {
            if (i->second.actor_)
            {
                i->second.actor_->SetVisibility(false);
                ren_->RemoveActor(i->second.actor_);
            }
            displayedData_.erase(i);
        }
    }
}



void IQCADModel3DViewer::addVertex(
        const QPersistentModelIndex& pidx,
        const QString& lbl,
         insight::cad::VectorPtr loc )
{
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );

    vtkNew<vtkPointSource> point;
    auto p = loc->value();
    point->SetCenter(p[0], p[1], p[2]);
    point->SetNumberOfPoints(1);
    point->SetRadius(0);
    actor->GetMapper()->SetInputConnection(point->GetOutputPort());
    auto prop=actor->GetProperty();
    prop->SetRepresentationToPoints();
    prop->SetPointSize(8);
    prop->SetColor(0, 0, 0);

    ren_->AddActor(actor);

    displayedData_[pidx]={lbl, loc, actor};
}



void IQCADModel3DViewer::addDatum(
        const QPersistentModelIndex& pidx,
        const QString& lbl,
        insight::cad::DatumPtr datum )
{
    vtkNew<vtkNamedColors> colors;

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
    actor->GetProperty()->SetColor(colors->GetColor3d("lightBlue").GetData());

    if (datum->providesPlanarReference())
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
    ren_->AddActor(actor);

    displayedData_[pidx]={lbl, datum, actor};
}




void IQCADModel3DViewer::addFeature(
        const QPersistentModelIndex& pidx,
        const QString& lbl,
        insight::cad::FeaturePtr feat )
{


    vtkNew<ivtkOCCShape> shape;
    shape->SetShape( feat->shape() );

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
    actor->GetMapper()->SetInputConnection(shape->GetOutputPort());

    ren_->AddActor(actor);

    displayedData_[pidx]={lbl, feat, actor};

    resetFeatureDisplayProps(pidx);
}



void IQCADModel3DViewer::resetVisibility(const QPersistentModelIndex &pidx)
{
    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
    {
        if (auto act = vtkActor::SafeDownCast(displayedData_[pidx].actor_))
        {
            QModelIndex idx(pidx);
            auto visible = idx.siblingAtColumn(IQCADItemModel::visibilityCol)
                    .data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
            act->SetVisibility(visible);
        }
    }
}





void IQCADModel3DViewer::resetFeatureDisplayProps(const QPersistentModelIndex& pidx)
{
    resetVisibility(pidx);

    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
    {
        if (auto act = vtkActor::SafeDownCast(displayedData_[pidx].actor_))
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




void IQCADModel3DViewer::resetDatasetColor(
        const QPersistentModelIndex& pidx )
{
    resetVisibility(pidx);

    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
    {
        auto de = displayedData_[pidx];

        if (auto actor = vtkActor::SafeDownCast(de.actor_))
        {
            vtkMapper* mapper=actor->GetMapper();
            vtkDataSet *pds = mapper->GetInput();

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
                double mima[2];
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




void IQCADModel3DViewer::addDataset(
        const QPersistentModelIndex& pidx,
        const QString& lbl,
        vtkSmartPointer<vtkDataObject> ds )
{
    vtkProp *act;

    if (auto ids = vtkImageData::SafeDownCast(ds) )
    {
        auto actor = vtkSmartPointer<vtkImageActor>::New();
        auto mapper = actor->GetMapper();
        mapper->SetInputData(ids);
        ren_->AddActor(actor);
        act = actor;
    }
    else if (auto pds = vtkDataSet::SafeDownCast(ds) )
    {
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkDataSetMapper>::New() );
        auto mapper=actor->GetMapper();

        mapper->SetInputDataObject(pds);
        ren_->AddActor(actor);
        act = actor;
    }
    else
    {
        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
        actor->GetMapper()->SetInputDataObject(ds);
        ren_->AddActor(actor);
        act = actor;
    }

    displayedData_[pidx]={lbl, ds, act};

    resetDatasetColor(pidx);
}




void IQCADModel3DViewer::onDataChanged(
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
                if ( feat.canConvert<insight::cad::FeaturePtr>()
                     && colInRange(IQCADItemModel::entityColorCol,
                                   IQCADItemModel::entityRepresentationCol) )
                {
                    resetFeatureDisplayProps(pidx);
                }
                if ( feat.canConvert<vtkSmartPointer<vtkDataObject> >()
                     && colInRange(IQCADItemModel::datasetFieldNameCol,
                                   IQCADItemModel::datasetRepresentationCol) )
                {
                    resetDatasetColor(pidx);
                }
            }

            if (roles.indexOf(Qt::CheckStateRole)>=0 || roles.empty())
            {
                if (colInRange(IQCADItemModel::visibilityCol))
                {
                    resetVisibility(pidx);
//                    bool vis = (idx.siblingAtColumn(IQCADItemModel::visibilityCol).data(Qt::CheckStateRole)
//                        .value<Qt::CheckState>() == Qt::Checked);
//                    displayedData_[pidx].actor_->SetVisibility(vis);
                }
            }

        }
    }
}




void IQCADModel3DViewer::onModelAboutToBeReset()
{}




void IQCADModel3DViewer::onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{}




void IQCADModel3DViewer::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    for (int r=start; r<=end; ++r)
    {
        addChild( model_->index(r, 0, parent) );
    }
}




void IQCADModel3DViewer::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    for (int r=first; r<=last; ++r)
    {
        remove( model_->index(r, 0, parent) );
    }
}



void IQCADModel3DViewer::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
}


void IQCADModel3DViewer::addChild(const QModelIndex& idx)
{
    QPersistentModelIndex pidx(idx);
    auto lbl = idx.siblingAtColumn(IQCADItemModel::labelCol).data().toString();
    auto feat = idx.siblingAtColumn(IQCADItemModel::entityCol).data();

    if (feat.canConvert<insight::cad::VectorPtr>())
    {
        addVertex( pidx, lbl, feat.value<insight::cad::VectorPtr>() );
    }
    else if (feat.canConvert<insight::cad::FeaturePtr>())
    {
        addFeature( pidx, lbl, feat.value<insight::cad::FeaturePtr>() );
    }
    else if (feat.canConvert<insight::cad::DatumPtr>())
    {
        addDatum( pidx, lbl, feat.value<insight::cad::DatumPtr>() );
    }
    else if (feat.canConvert<vtkSmartPointer<vtkDataObject> >())
    {
        addDataset( pidx, lbl, feat.value<vtkSmartPointer<vtkDataObject> >() );
    }

    resetVisibility(pidx);
}





void IQCADModel3DViewer::addSiblings(const QModelIndex& idx)
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




IQCADModel3DViewer::IQCADModel3DViewer(
        QWidget* parent )
    : QWidget(parent),
      vtkWidget_(this),
      model_( nullptr )
{
    insight::dbg()<<"add renderer"<<std::endl;
    insight::dbg()<<vtkWidget_.GetRenderWindow()<<std::endl;
    ren_ = vtkSmartPointer<vtkRenderer>::New();
    insight::dbg()<<ren_.GetPointer()<<std::endl;
    vtkWidget_.GetRenderWindow()->AddRenderer(ren_);
    insight::dbg()<<"set bg"<<std::endl;
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
    connect(fitBtn, &QPushButton::clicked, this,
            [&]() {
        ren_->ResetCamera();
    });

    auto setCam = [this](double upx, double upy, double upz,
                         double dx, double dy, double dz)
    {
        auto cam = ren_->GetActiveCamera();
        arma::mat cp, fp;
        cp=fp=insight::vec3Zero();
        cam->GetPosition(cp.memptr());
        cam->GetFocalPoint(fp.memptr());
        double L=arma::norm(fp-cp,2);
        cam->SetViewUp(upx,upy,upz);
        cam->SetPosition( arma::mat(fp - insight::vec3(dx,dy,dz)*L).memptr() );
    };

    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_plusx.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this,
                std::bind(setCam, 0,0,1, 1,0,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_minusx.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this,
                std::bind(setCam, 0,0,1, -1,0,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_plusy.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this,
                std::bind(setCam, 0,0,1, 0,1,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_minusy.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this,
                std::bind(setCam, 0,0,1, 0,-1,0));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_plusz.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this,
                std::bind(setCam, 0,1,0, 0,0,1));
    }
    {
        auto btn = new QPushButton(QPixmap(":/icons/icon_minusz.svg"), "");
        btn->setIconSize(QSize(24,24));
        btnLayout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this,
                std::bind(setCam, 0,1,0, 0,0,-1));
    }

    btnLayout->addItem(new QSpacerItem(1,1,QSizePolicy::Expanding));
    auto axes = vtkSmartPointer<vtkAxesActor>::New();

    auto renWin1 = vtkWidget_.GetRenderWindow();

    // Call vtkRenderWindowInteractor in orientation marker widgt
    auto widget = vtkOrientationMarkerWidget::New();
    widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes );
    widget->SetInteractor( renWin1->GetInteractor() );
    widget->SetViewport( 0.0, 0.0, 0.3, 0.3 );
    widget->SetEnabled( 1 );
    widget->InteractiveOn();

    vtkNew<IQPickInteractorStyle> style;
    connect(
        style, &IQPickInteractorStyle::contextMenuRequested, this,
        [this](vtkActor* actor)
        {

            auto i = std::find_if(
                        displayedData_.begin(),
                        displayedData_.end(),
                        [actor](const DisplayedData::value_type& e)
                        { return (e.second.actor_==actor); }
            );
            if (i!=displayedData_.end())
            {
                Q_EMIT contextMenuRequested(i->first, QCursor::pos());
            }
        }
    );
//    vtkNew<vtkInteractorStyleTrackballCamera> style;
    insight::dbg()<<"set default renderer"<<std::endl;
    style->SetDefaultRenderer(ren_);
    insight::dbg()<<"set interactor style"<<std::endl;
    renWin1->GetInteractor()->SetInteractorStyle(style);

    ren_->GetActiveCamera()->SetParallelProjection(true);

    insight::dbg()<<"reset cam"<<std::endl;
    ren_->ResetCamera();

}




void IQCADModel3DViewer::setModel(QAbstractItemModel* model)
{
    if (model_)
    {
         disconnect(model_, &QAbstractItemModel::dataChanged, 0, 0);
         disconnect(model_, &QAbstractItemModel::modelAboutToBeReset, 0, 0);
         disconnect(model_, &QAbstractItemModel::rowsAboutToBeRemoved, 0, 0);
         disconnect(model_, &QAbstractItemModel::rowsAboutToBeInserted, 0, 0);
         disconnect(model_, &QAbstractItemModel::rowsInserted, 0, 0);
    }

    model_=model;

    connect(model, &QAbstractItemModel::dataChanged,
            this, &IQCADModel3DViewer::onDataChanged);

    connect(model, &QAbstractItemModel::modelAboutToBeReset,
            this, &IQCADModel3DViewer::onModelAboutToBeReset);

    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &IQCADModel3DViewer::onRowsAboutToBeRemoved);

    connect(model, &QAbstractItemModel::rowsRemoved,
            this, &IQCADModel3DViewer::onRowsRemoved);

    connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
            this, &IQCADModel3DViewer::onRowsAboutToBeInserted);
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &IQCADModel3DViewer::onRowsInserted);

    addSiblings(QModelIndex());
}

vtkRenderWindowInteractor *IQCADModel3DViewer::interactor() const
{
    return vtkWidget_.GetInteractor();
}




QSize IQCADModel3DViewer::sizeHint() const
{
    return QSize(1024,768);
}




//vtkActor *IQCADModel3DViewer::getActor(insight::cad::FeaturePtr geom)
//{
//    CADEntity e(geom);
//    auto i=displayedData_.find(e);
//    if (i!=displayedData_.end())
//    {
//        return vtkActor::SafeDownCast(i->second.actor_);
//    }
//    return nullptr;
//}

