
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

#include "cadfeature.h"


defineType(IQSpatialTransformationParameter);
addToFactoryTable(IQParameter, IQSpatialTransformationParameter);




IQSpatialTransformationParameter::IQSpatialTransformationParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSpecializedParameter<insight::SpatialTransformationParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{
}




QString IQSpatialTransformationParameter::valueText() const
{
  if (parameter()().isIdentityTransform())
      return QString("identity");
  else
  {
      arma::mat tr=parameter()().translate();
      bool trZero = arma::norm(tr, 2)<insight::SMALL;
      arma::mat rpy=parameter()().rollPitchYaw();

      QStringList shortDescActions;

      if (!trZero)
          shortDescActions<<QString("+[%1 %2 %3]").arg(tr(0)).arg(tr(1)).arg(tr(2));

      if (fabs(rpy(0))>insight::SMALL)
          shortDescActions<<QString("Roll %1°").arg(tr(0));

      if (fabs(rpy(1))>insight::SMALL)
          shortDescActions<<QString("Pitch %1°").arg(tr(1));

      if (fabs(rpy(2))>insight::SMALL)
          shortDescActions<<QString("Yaw %1°]").arg(tr(2));

      if (fabs(parameter()().scale()-1.)>insight::SMALL)
          shortDescActions<<QString("*%1").arg(parameter()().scale());

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

    arma::mat m;

    insight::SpatialTransformation st;
    insight::stringToValue(translateLE->text().toStdString(), m);
    st.setTranslation(m);

    insight::stringToValue(rpyLE->text().toStdString(), m);
    st.setRollPitchYaw(m);

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
            if (insight::cad::FeaturePtr geom =
              model()->getGeometryToSpatialTransformationParameter(get()->path()))
            {
                vtkNew<ivtkOCCShape> shape;
                shape->SetShape( geom->shape() );
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
                    auto tini = p().toVTKTransform();

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
                }
            }
          }
    );
  }

  return layout;
}
