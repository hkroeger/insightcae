#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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
    IQParameterSetModel* psmodel,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
) : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{}


QString IQVectorParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::VectorParameter&>(parameter());

  return "["+QString::fromStdString(insight::valueToString(p()))+"]";
}



QVBoxLayout* IQVectorParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto& p= dynamic_cast<const insight::VectorParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  lineEdit=new QLineEdit(editControlsContainer);

  lineEdit->setText( QString::fromStdString(insight::valueToString(p())) );

  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

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
      auto& p = dynamic_cast<insight::VectorParameter&>(this->parameterRef());
      arma::mat v;
      insight::stringToValue(lineEdit->text().toStdString(), v);
      p.set(v);
//      model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

#warning need generalized implementation
  if (auto *v = dynamic_cast<IQVTKCADModel3DViewer*>(viewer))
  {
    connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [this,v,apply,applyFunction]()
          {
            const auto& p =
                    dynamic_cast<const insight::VectorParameter&>(
                        parameter() );
            if (auto bp =
                model()->getVectorBasePoint(path()))
            {
                // auto curMod =
                //       new IQVectorDirectionCommand(
                //             v->interactor(),
                //             (*bp), p() );

                // connect( apply, &QPushButton::pressed,
                //          curMod, &QObject::deleteLater );

                // connect( curMod, &IQVectorDirectionCommand::dataChanged, curMod,
                //          [this,curMod]()
                //          {
                //            lineEdit->setText(
                //                        QString::fromStdString(
                //                            insight::valueToString(
                //                                curMod->getVector()
                //                                ) ) );
                //          } );
                auto mani = std::make_shared<IQVTKManipulateCoordinateSystem>(
                    *v, insight::CoordinateSystem((*bp), p()), true );
                connect(mani.get(), &IQVTKManipulateCoordinateSystem::coordinateSystemSelected,
                        [this,applyFunction](const insight::CoordinateSystem& cs)
                        {
                            auto& p = dynamic_cast<insight::VectorParameter&>(
                                this->parameterRef());
                            p.set(cs.ex);
                        }
                        );
                v->launchAction(mani);
            }
            else
            {
              auto ppc = std::make_shared<IQVTKCADModel3DViewerPickPoint>(*v);
              connect(ppc.get(), &IQVTKCADModel3DViewerPickPoint::pickedPoint,
                        [this,applyFunction](const arma::mat& pt)
                        {
                          auto& p = dynamic_cast<insight::VectorParameter&>(
                              this->parameterRef());
                          p.set(pt);
                          // lineEdit->setText(
                          //     QString::fromStdString(
                          //         insight::valueToString(p) ) );
                          // applyFunction();
                        }
                      );
              v->launchAction(ppc);
            }
          }
    );
  }

  return layout;
}

void IQVectorParameter::applyProposition(
    const insight::ParameterSet &propositions,
    const std::string &selectedProposition)
{
  const auto& pp=propositions.get<insight::VectorParameter>(selectedProposition);
  auto& p= dynamic_cast<insight::VectorParameter&>(this->parameterRef());
  p=pp;
  lineEdit->setText( QString::fromStdString(insight::valueToString(p())) );
//  model->notifyParameterChange(index);
}
