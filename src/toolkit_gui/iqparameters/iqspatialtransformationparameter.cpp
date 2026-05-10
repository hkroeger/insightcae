
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>

#include "vtkProperty.h"
#include "vtkTransform.h"
#include "vtkPolyDataMapper.h"

#include "iqspatialtransformationparameter.h"
#include "iqparametersetmodel.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqcadtransformationcommand.h"
#include "ivtkoccshape.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkmanipulatecoordinatesystem.h"

#include "cadfeature.h"


defineType(IQSpatialTransformationParameter);
addToFactoryTable(IQParameter, IQSpatialTransformationParameter);




IQSpatialTransformationParameter::IQSpatialTransformationParameter
(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
)
  : IQSpecializedParameter<insight::SpatialTransformationParameter>(
          parent, hdmodel, element)
{
}




QVariant IQSpatialTransformationParameter::value() const
{
  if (parameter()().isIdentityTransform())
      return QString("identity");
  else
  {

      QStringList shortDescActions;

      {
          arma::mat tr=parameter()().translate();
          if (arma::norm(tr, 2)>insight::SMALL)
              shortDescActions<<QString("+[%1 %2 %3]").arg(tr(0)).arg(tr(1)).arg(tr(2));
      }

      {
          arma::mat rpy=parameter()().rollPitchYaw();
          if (fabs(rpy(0))>insight::SMALL)
              shortDescActions<<QString("Roll %1°").arg(rpy(0));

          if (fabs(rpy(1))>insight::SMALL)
              shortDescActions<<QString("Pitch %1°").arg(rpy(1));

          if (fabs(rpy(2))>insight::SMALL)
              shortDescActions<<QString("Yaw %1°").arg(rpy(2));
      }

      {
          if (fabs(parameter()().scale()-1.)>insight::SMALL)
              shortDescActions<<QString("*%1").arg(parameter()().scale());
      }

      return shortDescActions.join(" > ");
  }
}





QVBoxLayout* IQSpatialTransformationParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer )
{
  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QLineEdit *translateLE, *rpyLE, *scaleLE;

  {
      QHBoxLayout *layout2=new QHBoxLayout;
      QLabel *translateLabel = new QLabel("Translation:", editControlsContainer);
      layout2->addWidget(translateLabel);
      translateLE = new QLineEdit(editControlsContainer);
      layout2->addWidget(translateLE);
      layout->addLayout(layout2);
  }

  {
      QHBoxLayout *layout2=new QHBoxLayout;
      QLabel *rpyLabel = new QLabel("Roll Pitch Yaw:", editControlsContainer);
      layout2->addWidget(rpyLabel);
      rpyLE = new QLineEdit(editControlsContainer);
      layout2->addWidget(rpyLE);
      layout->addLayout(layout2);
  }

  {
      QHBoxLayout *layout2=new QHBoxLayout;
      QLabel *scaleLabel = new QLabel("Scale factor:", editControlsContainer);
      layout2->addWidget(scaleLabel);
      scaleLE = new QLineEdit(editControlsContainer);
      layout2->addWidget(scaleLE);
      layout->addLayout(layout2);
  }

  auto setValuesToControls =
          [translateLE,rpyLE,scaleLE](const insight::SpatialTransformation& t)
  {
      translateLE->setText(mat2Str(t.translate()));
      rpyLE->setText(mat2Str(t.rollPitchYaw()));
      scaleLE->setText(QString::number(t.scale()));
  };

  setValuesToControls(parameter()());

  QPushButton *dlgBtn_=nullptr;
  if (viewer)
  {
      dlgBtn_ = new QPushButton("...", editControlsContainer);
      layout->addWidget(dlgBtn_);
  }

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {

    insight::SpatialTransformation st;
    st.setTranslation(
        insight::toValue<arma::mat>(
            translateLE->text().toStdString()));

    st.setRollPitchYaw(
        insight::toValue<arma::mat>(
            rpyLE->text().toStdString()));

    bool ok;
    st.setScale(scaleLE->text().toDouble(&ok));
    insight::assertion(ok, "invalid input for scale factor!");

    parameterRef().set(st);
  };

  connect(translateLE, &QLineEdit::returnPressed, applyFunction);
  connect(rpyLE, &QLineEdit::returnPressed, applyFunction);
  connect(scaleLE, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  if (auto *v = dynamic_cast<IQVTKCADModel3DViewer*>(viewer))
  {
    connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [this,translateLE,v,setValuesToControls,apply]()
          {
            if (auto cd =
              psModel()->GUIContext()->getData<insight::SpatialTransformationParameter>(get()->path()))
            {
                vtkNew<ivtkOCCShape> shape;
                shape->SetShape( cd->geometry->shape() );
                auto actor = vtkSmartPointer<vtkActor>::New();
                actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
                actor->GetMapper()->SetInputConnection(shape->GetOutputPort());
                actor->GetProperty()->SetOpacity(0.7);
                actor->GetProperty()->SetColor(0.7, 0.3, 0.3);

//                if (vtkActor* actor = v->getActor(geom))
                const auto& p =
                        dynamic_cast<const insight::SpatialTransformationParameter&>(
                            parameter() );
                {
                    auto initialCS =
                        ( parameter()() * cd->referenceCS.localToGlobal() )
                                         .localCoordinateSystem();

                    auto mani = make_viewWidgetAction<IQVTKManipulateCoordinateSystem>(
                        *v->topmostActionHost(), initialCS );

                    connect(mani.get(), &IQVTKManipulateCoordinateSystem::coordinateSystemSelected,
                            [this,cd](const insight::CoordinateSystem& newCS)
                            {
                                auto stc=newCS.localToGlobal();

                                auto newTr = stc * cd->referenceCS.localToGlobal().inverted();

                                parameterRef().set( newTr );
                            }
                            );
                    v->topmostActionHost()->launchAction(std::move(mani));

                    /*

                  auto curMod =
                        new IQCADTransformationCommand(
                              actor, v->interactor(),
                              tini, true, true, false );

                  connect( translateLE, &QObject::destroyed,
                           curMod, &QObject::deleteLater );

                  connect( apply, &QPushButton::pressed,
                           curMod, &QObject::deleteLater );

                  connect( curMod, &IQCADTransformationCommand::dataChanged, this,
                           [this,curMod,setValuesToControls]()
                           {
                             setValuesToControls(curMod->getSpatialTransformation());
                           } );
*/
                }
            }
          }
    );
  }

  return layout;
}
