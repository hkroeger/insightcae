#include "iqcadmodel3dviewer.h"

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
#include "ivtkoccshape.h"

#include "datum.h"


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







void IQCADModel3DViewer::remove(std::map<CADEntity, DisplayedEntity>::iterator i)
{
    ren_->RemoveActor(i->second.actor_);
    displayedData_.erase(i);
}




IQCADModel3DViewer::DisplayedEntity& IQCADModel3DViewer::addDatum(const QString& lbl, insight::cad::DatumPtr datum)
{
    vtkNew<vtkNamedColors> colors;

    DisplayedEntity de;
    de.label_=lbl;
    de.mapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    de.actor_ = vtkSmartPointer<vtkActor>::New();
    de.actor_->GetProperty()->SetColor(colors->GetColor3d("lightBlue").GetData());

    if (datum->providesPlanarReference())
    {
        vtkNew<vtkPlaneSource> plane;
        auto pl = datum->plane();
        plane->SetCenter(pl.Location().X(), pl.Location().Y(), pl.Location().Z());
        plane->SetNormal(pl.Direction().X(), pl.Direction().Y(), pl.Direction().Z());
        de.mapper_->SetInputConnection(plane->GetOutputPort());
        de.actor_->GetProperty()->SetOpacity(0.33);
    }
    else if (datum->providesAxisReference())
    {
        vtkNew<vtkLineSource> line;
        auto ax = datum->axis();
        gp_Pnt p1=ax.Location();
        gp_Pnt p2=p1.Translated(ax.Direction());
        line->SetPoint1(p1.X(), p1.Y(), p1.Z());
        line->SetPoint2(p2.X(), p2.Y(), p2.Z());
        de.mapper_->SetInputConnection(line->GetOutputPort());
        de.actor_->GetProperty()->SetRepresentationToWireframe();
        de.actor_->GetProperty()->SetLineStipplePattern(0xc003);
    }

    de.actor_->SetMapper(de.mapper_);
    ren_->AddActor(de.actor_);
    return displayedData_.insert({datum, de}).first->second;
}




IQCADModel3DViewer::DisplayedEntity& IQCADModel3DViewer::addFeature(
        const QString& lbl,
        insight::cad::FeaturePtr feat )
{
    vtkNew<ivtkOCCShape> shape;
    shape->SetShape( feat->shape() );
    DisplayedEntity de;
    de.label_=lbl;
    de.mapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    de.mapper_->SetInputConnection(shape->GetOutputPort());
    de.actor_ = vtkSmartPointer<vtkActor>::New();
    de.actor_->SetMapper(de.mapper_);
    ren_->AddActor(de.actor_);
    return displayedData_.insert({feat, de}).first->second;
}



IQCADModel3DViewer::DisplayedEntity& IQCADModel3DViewer::addDataset(
        const QString& lbl,
        vtkSmartPointer<vtkDataObject> ds )
{
    DisplayedEntity de;
    de.label_=lbl;
    de.mapper_ = vtkSmartPointer<vtkDataSetMapper>::New();
    de.mapper_->SetInputDataObject(ds);
    de.actor_ = vtkSmartPointer<vtkActor>::New();
    de.actor_->SetMapper(de.mapper_);
    ren_->AddActor(de.actor_);
    return displayedData_.insert({ds, de}).first->second;
}



void IQCADModel3DViewer::onDataChanged(
        const QModelIndex &topLeft,
        const QModelIndex &bottomRight,
        const QVector<int> &roles )
{
    qDebug()<<"onDataChanged"<<topLeft<<bottomRight<<roles;
    if (roles.empty() || roles.indexOf(Qt::CheckStateRole)>=0)
    {
        for (int r=topLeft.row(); r<=bottomRight.row(); ++r)
        {
            auto vis = model_->data(model_->index(r, visibilityCol_, topLeft.parent()), Qt::CheckStateRole).toBool();
            auto lbl = model_->data(model_->index(r, labelCol_, topLeft.parent())).toString();
            auto feat = model_->data(model_->index(r, entityCol_, topLeft.parent()));
            if (feat.canConvert<insight::cad::FeaturePtr>())
            {
                qDebug()<<"visibility feat"<<lbl<<vis<<feat;
                auto f = feat.value<insight::cad::FeaturePtr>();
                auto i = displayedData_.find(f);
                i->second.actor_->SetVisibility(vis);
                this->renderWindow()->Render();
            }
            else if (feat.canConvert<insight::cad::DatumPtr>())
            {
                qDebug()<<"visibility datum"<<lbl<<vis<<feat;
                auto f = feat.value<insight::cad::DatumPtr>();
                auto i = displayedData_.find(f);
                i->second.actor_->SetVisibility(vis);
                this->renderWindow()->Render();
            }
            else if (feat.canConvert<vtkSmartPointer<vtkDataObject> >())
            {
                qDebug()<<"visibility dataset"<<lbl<<vis<<feat;
                auto f = feat.value<vtkSmartPointer<vtkDataObject> >();
                auto i = displayedData_.find(f);
                i->second.actor_->SetVisibility(vis);
                this->renderWindow()->Render();
            }
        }
    }
}

void IQCADModel3DViewer::onModelAboutToBeReset()
{
    qDebug()<<"onModelAboutToBeReset";
}

void IQCADModel3DViewer::onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    qDebug()<<"onRowsAboutToBeInserted"<<parent<<start<<end;
}

void IQCADModel3DViewer::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    qDebug()<<"onRowsInserted"<<parent<<start<<end;
    for (int r=start; r<=end; ++r)
    {
        addChild( model_->index(r, labelCol_, parent) );
    }
}

void IQCADModel3DViewer::onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    qDebug()<<"onRowsAboutToBeRemoved"<<parent<<first<<last;
}


void IQCADModel3DViewer::addChild(const QModelIndex& i)
{
    int r=i.row();
    QModelIndex idx=i.parent();

    bool vis = model_->data(model_->index(r, visibilityCol_, idx), Qt::CheckStateRole).toBool();
    auto lbl = model_->data( i ).toString();
    auto feat = model_->data( model_->index(r, entityCol_, idx) );
    qDebug()<<"add child"<<lbl<<r;
    if (feat.canConvert<insight::cad::FeaturePtr>())
    {
        addFeature( lbl, feat.value<insight::cad::FeaturePtr>() ).actor_->SetVisibility(vis);
    }
    else if (feat.canConvert<insight::cad::DatumPtr>())
    {
        addDatum( lbl, feat.value<insight::cad::DatumPtr>() ).actor_->SetVisibility(vis);
    }
    else if (feat.canConvert<vtkSmartPointer<vtkDataObject> >())
    {
        addDataset( lbl, feat.value<vtkSmartPointer<vtkDataObject> >() ).actor_->SetVisibility(vis);
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
        QWidget* parent,
        int visibilityCol, int labelCol, int entityCol )
    : VTKWidget(parent),
      visibilityCol_(visibilityCol),
      labelCol_(labelCol),
      entityCol_(entityCol),
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
    style->SetDefaultRenderer(ren_);
    renWin1->GetInteractor()->SetInteractorStyle(style);

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

