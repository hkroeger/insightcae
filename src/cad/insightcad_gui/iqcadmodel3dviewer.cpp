#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "iscadmetatyperegistrator.h"

#include <QDebug>

#include <vtkAxesActor.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkDataSetMapper.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPlaneSource.h>
#include <vtkLineSource.h>
#include <vtkProperty.h>
#include <vtkNamedColors.h>
#include <vtkPropPicker.h>
#include <vtkCubeSource.h>
#include "vtkImageMapper3D.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "ivtkoccshape.h"

#include "datum.h"

#include "base/vtkrendering.h"


vtkStandardNewMacro(PickInteractorStyle);


PickInteractorStyle::PickInteractorStyle()
    : vtkInteractorStyleTrackballCamera()
{
    lastClickPos[0]=lastClickPos[1]=0;
}


void PickInteractorStyle::OnLeftButtonUp()
{

  int clickPos[2];
  this->GetInteractor()->GetEventPosition(clickPos[0], clickPos[1]);

  if (clickPos[0]==lastClickPos[0] && clickPos[1]==lastClickPos[1])
  {
      // Pick from this location.
      vtkNew<vtkPropPicker> picker;
      picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

      double* pos = picker->GetPickPosition();
      std::cout
              << "Pick position (world coordinates) is: "
              << pos[0] << " " << pos[1] << " " << pos[2]
              << std::endl;

      auto pickedActor = picker->GetActor();

      if (pickedActor == nullptr)
      {
        std::cout << "No actor picked." << std::endl;

        HighlightProp(nullptr);
      }
      else
      {
        auto pac=picker->GetActor();
        std::cout << "Picked actor: " << pac << std::endl;

        HighlightProp( picker->GetProp3D() );
      }
  }

  vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
}


void PickInteractorStyle::OnLeftButtonDown()
{

  this->GetInteractor()->GetEventPosition(
              lastClickPos[0], lastClickPos[1] );

  vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}



void PickInteractorStyle::OnRightButtonUp()
{

  int clickPos[2];
  this->GetInteractor()->GetEventPosition(clickPos[0], clickPos[1]);

  if (clickPos[0]==lastClickPos[0] && clickPos[1]==lastClickPos[1])
  {
      // Pick from this location.
      vtkNew<vtkPropPicker> picker;
      picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

      double* pos = picker->GetPickPosition();
      std::cout
              << "Pick position (world coordinates) is: "
              << pos[0] << " " << pos[1] << " " << pos[2]
              << std::endl;

      auto pickedActor = picker->GetActor();

      if (pickedActor == nullptr)
      {
        std::cout << "No actor picked." << std::endl;

        HighlightProp(nullptr);
      }
      else
      {
        auto pac=picker->GetActor();
        std::cout << "Picked actor RM: " << pac << std::endl;

        Q_EMIT contextMenuRequested(pac);

        HighlightProp( picker->GetProp3D() );
      }
  }

  vtkInteractorStyleTrackballCamera::OnRightButtonUp();
}


void PickInteractorStyle::OnRightButtonDown()
{

  this->GetInteractor()->GetEventPosition(
              lastClickPos[0], lastClickPos[1] );

  vtkInteractorStyleTrackballCamera::OnRightButtonDown();
}









void IQCADModel3DViewer::remove(const QPersistentModelIndex& pidx)
{
//    qDebug()<<"remove"<<pidx;
    if (pidx.isValid())
    {
        auto i = displayedData_.find(pidx);
        if (i!=displayedData_.end())
        {
            i->second.actor_->SetVisibility(false);
            ren_->RemoveActor(i->second.actor_);
            displayedData_.erase(i);
        }
    }
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
        vtkNew<vtkLineSource> line;
        auto ax = datum->axis();
        gp_Pnt p1=ax.Location();
        gp_Pnt p2=p1.Translated(ax.Direction());
        line->SetPoint1(p1.X(), p1.Y(), p1.Z());
        line->SetPoint2(p2.X(), p2.Y(), p2.Z());
        actor->GetMapper()->SetInputConnection(line->GetOutputPort());
        actor->GetProperty()->SetRepresentationToWireframe();
        actor->GetProperty()->SetLineStipplePattern(0xc003);
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




void IQCADModel3DViewer::resetFeatureDisplayProps(const QPersistentModelIndex& pidx)
{
    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
    {
        QModelIndex idx(pidx);
        if (auto act = vtkActor::SafeDownCast(displayedData_[pidx].actor_))
        {
            auto opacity = idx.siblingAtColumn(IQCADItemModel::entityOpacityCol)
                    .data().toDouble();
            act->GetProperty()->SetOpacity(opacity);

            auto visible = idx.siblingAtColumn(IQCADItemModel::visibilityCol)
                    .data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
            act->SetVisibility(visible);

            auto color = idx.siblingAtColumn(IQCADItemModel::entityColorCol)
                    .data().value<QColor>();
            act->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
        }
    }
}




void IQCADModel3DViewer::resetDatasetColor(
        const QPersistentModelIndex& pidx )
{
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

                std::cout<<mima[0]<<" -> "<<mima[1]<<std::endl;

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




//QModelIndex IQCADModel3DViewer::modelIndex(const CADEntity &ce) const
//{
//    if (auto *cadmodel = dynamic_cast<IQCADItemModel*>(model_))
//    {
//        auto di = displayedData_.find(ce);
//        if (boost::get<insight::cad::FeaturePtr>(&ce))
//        {
//            return cadmodel->modelstepIndex(di->second.label_.toStdString());
//        }
//        else if (boost::get<insight::cad::DatumPtr>(&ce))
//        {
//            return cadmodel->datumIndex(di->second.label_.toStdString());
//        }
//        else if (boost::get<vtkSmartPointer<vtkDataObject> >(&ce))
//        {
//            return cadmodel->datasetIndex(di->second.label_.toStdString());
//        }
//    }
//    return QModelIndex();
//}




void IQCADModel3DViewer::onDataChanged(
        const QModelIndex &topLeft,
        const QModelIndex &bottomRight,
        const QVector<int> &roles )
{
//    qDebug()<<"onDataChanged"<<topLeft<<bottomRight<<roles;
    if (roles.empty() || roles.indexOf(Qt::CheckStateRole)>=0 || roles.indexOf(Qt::EditRole)>=0)
    {
        for (int r=topLeft.row(); r<=bottomRight.row(); ++r)
        {
            auto idx = model_->index(r, 0, topLeft.parent());
            QPersistentModelIndex pidx(idx);

//            qDebug()<<idx<<pidx<<displayedData_.begin()->first;

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
                                   IQCADItemModel::entityOpacityCol) )
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
                    bool vis = (idx.siblingAtColumn(IQCADItemModel::visibilityCol).data(Qt::CheckStateRole)
                        .value<Qt::CheckState>() == Qt::Checked);
                    displayedData_[pidx].actor_->SetVisibility(vis);
                }
            }

        }
    }
}




void IQCADModel3DViewer::onModelAboutToBeReset()
{
//    qDebug()<<"onModelAboutToBeReset";
}




void IQCADModel3DViewer::onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
//    qDebug()<<"onRowsAboutToBeInserted"<<parent<<start<<end;
}




void IQCADModel3DViewer::onRowsInserted(const QModelIndex &parent, int start, int end)
{
//    qDebug()<<"onRowsInserted"<<parent<<start<<end;
    for (int r=start; r<=end; ++r)
    {
        addChild( model_->index(r, 0, parent) );
    }
}




void IQCADModel3DViewer::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
//    qDebug()<<"onRowsAboutToBeRemoved"<<parent<<first<<last;
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
    QPersistentModelIndex i(idx);
    auto lbl = idx.siblingAtColumn(IQCADItemModel::labelCol).data().toString();
    auto feat = idx.siblingAtColumn(IQCADItemModel::entityCol).data();

//    qDebug()<<"add child"<<lbl<<r;
    if (feat.canConvert<insight::cad::FeaturePtr>())
    {
        addFeature( i, lbl, feat.value<insight::cad::FeaturePtr>() );
    }
    else if (feat.canConvert<insight::cad::DatumPtr>())
    {
        addDatum( i, lbl, feat.value<insight::cad::DatumPtr>() );
    }
    else if (feat.canConvert<vtkSmartPointer<vtkDataObject> >())
    {
        addDataset( i, lbl, feat.value<vtkSmartPointer<vtkDataObject> >() );
    }
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
    : VTKWidget(parent),
      model_(nullptr),
      ren_(decltype(ren_)::New())
{
    ren_->SetBackground(1., 1., 1.);
    renderWindow()->AddRenderer(ren_);
    auto axes = vtkSmartPointer<vtkAxesActor>::New();

    auto renWin1 = renderWindow();

    // Call vtkRenderWindowInteractor in orientation marker widgt
    auto widget = vtkOrientationMarkerWidget::New();
    widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes );
    widget->SetInteractor( renWin1->GetInteractor() );
    widget->SetViewport( 0.0, 0.0, 0.3, 0.3 );
    widget->SetEnabled( 1 );
    widget->InteractiveOn();

    vtkNew<PickInteractorStyle> style;
//    vtkNew<vtkInteractorStyleTrackballCamera> style;
    style->SetDefaultRenderer(ren_);
    renWin1->GetInteractor()->SetInteractorStyle(style);

    ren_->ResetCamera();

    connect(
        style, &PickInteractorStyle::contextMenuRequested, this,
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

