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
    IQParameterSetModel* psmodel,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{
}


QString IQBoolParameter::valueText() const
{
  const auto&p = dynamic_cast<const insight::BoolParameter&>(parameter());
  return p() ? "true" : "false";
}



QVBoxLayout* IQBoolParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto& p = dynamic_cast<insight::BoolParameter&>(parameterRef());

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
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


  auto applyFunction = [this,&p,checkBox]()
  {
    p.set(checkBox->checkState() == Qt::Checked);
//    model->notifyParameterChange(index);
  };

  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  auto connection = p.valueChanged.connect([&p,checkBox](){
      QSignalBlocker sb(checkBox);
      checkBox->setCheckState(p()?Qt::Checked:Qt::Unchecked);
  });
  connect(layout, &QObject::destroyed, layout,
          [connection](){connection.disconnect(); });

  return layout;
}
