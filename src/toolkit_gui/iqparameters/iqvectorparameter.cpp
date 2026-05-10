#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "qtextensions.h"
#include <memory>

#include "iqvectorparameter.h"
#include "base/linearalgebra.h"
#include "iqparametersetmodel.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvectordirectioncommand.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerpickpoint.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkmanipulatecoordinatesystem.h"




defineType(IQVectorParameter);
addToFactoryTable(IQParameter, IQVectorParameter);




IQVectorParameter::IQVectorParameter
(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
) : IQSpecializedParameter<insight::VectorParameter>(
          parent, hdmodel, element)
{}






QVBoxLayout* IQVectorParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  lineEdit=new QLineEdit(editControlsContainer);

  lineEdit->setText( QString::fromStdString(
      insight::toString(parameter()()) ));

  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  QPushButton *dlgBtn_=nullptr;
  if (viewer &&
      (parameter().vectorType()!=insight::VectorParameter::NonSpatial))
  {
      dlgBtn_ = new QPushButton("...", editControlsContainer);
      layout->addWidget(dlgBtn_);
  }

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
      parameterRef().set(
          insight::toValue<arma::mat>(
              lineEdit->text()
                  .toStdString()));
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  if (auto *v = dynamic_cast<IQVTKCADModel3DViewer*>(viewer))
  {
    connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [this,v,apply,applyFunction]()
          {

          switch (parameter().vectorType())
          {
          case insight::VectorParameter::VectorType::Direction: {
              arma::mat O=insight::vec3Zero();
              if (auto bp =
                  psModel()->GUIContext()->getData<insight::VectorParameter>(parameter().path()))
              {
                  O=bp->basePoint;
              }

              auto mani = make_viewWidgetAction<IQVTKManipulateCoordinateSystem>(
                  *v->topmostActionHost(), insight::CoordinateSystem(O, parameter()()),
                    true,
                    IQVTKManipulateCoordinateSystem::ActorMask{
                        IQVTKManipulateCoordinateSystem::Ex,
                        IQVTKManipulateCoordinateSystem::XZ,
                        IQVTKManipulateCoordinateSystem::XY } );

              connect(mani.get(), &IQVTKManipulateCoordinateSystem::coordinateSystemSelected,
                      [this,applyFunction](const insight::CoordinateSystem& cs)
                      {
                          parameterRef().set(cs.ex);
                      }
                      );
              v->topmostActionHost()->launchAction(std::move(mani));

          } break;

          case insight::VectorParameter::VectorType::Point: {
              auto ppc = make_viewWidgetAction<IQVTKCADModel3DViewerPickPoint>(
                  *v->topmostActionHost(), true );

              connect(ppc.get(), &IQVTKCADModel3DViewerPickPoint::pickedPoint,
                      [this](const arma::mat& pt)
                      {
                          parameterRef().set(pt);
                      }
                      );
              v->topmostActionHost()->launchAction(std::move(ppc));
          } break;

          default: {}
          }
      }
    );
  }

  // handle external value change
  ::disconnectAtEOL(
      layout,
      parameterRef().valueChanged.connect(
          [this]()
          {
              DBG_SLOT(valueChanged);

              QSignalBlocker sb(lineEdit);
              lineEdit->setText( QString::fromStdString(
                  insight::toString(parameter()()) ) );
          }
          )
      );

  return layout;
}

void IQVectorParameter::applyProposition(
    const insight::ParameterSet &propositions,
    const std::string &selectedProposition)
{
    const auto& pp=
        propositions.get<insight::VectorParameter>(
        selectedProposition);

    parameterRef().assignFrom(pp);

    lineEdit->setText(
        QString::fromStdString(
            insight::toString(
                parameter()())) );
}
