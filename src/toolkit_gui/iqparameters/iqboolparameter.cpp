#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#include "iqboolparameter.h"
#include "iqparametersetmodel.h"

defineType(IQBoolParameter);
addToFactoryTable(IQParameter, IQBoolParameter);

IQBoolParameter::IQBoolParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQBoolParameter::valueText() const
{
  const auto&p = dynamic_cast<const insight::BoolParameter&>(parameter());
  return p() ? "true" : "false";
}



QVBoxLayout* IQBoolParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  const auto& p = dynamic_cast<const insight::BoolParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer);

  QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto* checkBox=new QCheckBox(editControlsContainer);
//  connect(checkBox, &QCheckBox::destroyed, this, &BoolParameterWrapper::onDestruction);
  if (p())
    checkBox->setCheckState(Qt::Checked);
  else
    checkBox->setCheckState(Qt::Unchecked);
  layout2->addWidget(checkBox);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);

  layout->addStretch();

  auto applyFunction = [=]()
  {
    auto &p = dynamic_cast<insight::BoolParameter&>(model->parameterRef(index));
    p() = (checkBox->checkState() == Qt::Checked);
    model->notifyParameterChange(index);
  };

  connect(apply, &QPushButton::pressed, applyFunction);

  return layout;
}
