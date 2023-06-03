#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "iqvectorparameter.h"
#include "iqparametersetmodel.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvectordirectioncommand.h"
#include "iqvtkvieweractions/iqvtkcadmodel3dviewerpickpoint.h"

defineType(IQVectorParameter);
addToFactoryTable(IQParameter, IQVectorParameter);

IQVectorParameter::IQVectorParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
) : IQParameter(parent, name, parameter, defaultParameterSet)
{}


QString IQVectorParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::VectorParameter&>(parameter());

  return "["+QString::fromStdString(insight::valueToString(p()))+"]";
}



QVBoxLayout* IQVectorParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto& p= dynamic_cast<const insight::VectorParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

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
    auto& p = dynamic_cast<insight::VectorParameter&>(model->parameterRef(index));
    insight::stringToValue(lineEdit->text().toStdString(), p());
    model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

#warning need generalized implementation
  if (auto *v = dynamic_cast<IQVTKCADModel3DViewer*>(viewer))
  {
    connect(dlgBtn_, &QPushButton::clicked, dlgBtn_,
          [this,model,v,apply,applyFunction]()
          {
            const auto& p =
                    dynamic_cast<const insight::VectorParameter&>(
                        parameter() );
            if (auto bp =
                    model->getVectorBasePoint(path()))
            {
                auto curMod =
                      new IQVectorDirectionCommand(
                            v->interactor(),
                            (*bp), p() );

                connect( apply, &QPushButton::pressed,
                         curMod, &QObject::deleteLater );

                connect( curMod, &IQVectorDirectionCommand::dataChanged, curMod,
                         [this,curMod]()
                         {
                           lineEdit->setText(
                                       QString::fromStdString(
                                           insight::valueToString(
                                               curMod->getVector()
                                               ) ) );
                         } );
            }
            else
            {
              auto ppc = std::make_shared<IQVTKCADModel3DViewerPickPoint>(*v);
              connect(ppc.get(), &IQVTKCADModel3DViewerPickPoint::pickedPoint,
                        [this,applyFunction](const arma::mat& p)
                        {
                          lineEdit->setText(
                              QString::fromStdString(
                                  insight::valueToString(p) ) );
                          applyFunction();
                        }
                      );
              v->launchUserActivity(ppc);
            }
          }
    );
  }

  return layout;
}

void IQVectorParameter::applyProposition(
    IQParameterSetModel* model, const QModelIndex &index,
    const insight::ParameterSet &propositions,
    const std::string &selectedProposition)
{
  const auto& pp=propositions.get<insight::VectorParameter>(selectedProposition);
  auto& p= dynamic_cast<insight::VectorParameter&>(model->parameterRef(index));
  p.reset(pp);
  lineEdit->setText( QString::fromStdString(insight::valueToString(p())) );
  model->notifyParameterChange(index);
}
