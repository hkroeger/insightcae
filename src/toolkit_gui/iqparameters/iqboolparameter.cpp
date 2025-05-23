#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#include "iqboolparameter.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"

defineType(IQBoolParameter);
addToFactoryTable(IQParameter, IQBoolParameter);

IQBoolParameter::IQBoolParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSpecializedParameter<insight::BoolParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{
}


QString IQBoolParameter::valueText() const
{
  return parameter()() ? "true" : "false";
}

bool IQBoolParameter::setValue(QVariant value)
{
    if (value.canConvert<Qt::CheckState>())
    {
        parameterRef().set(
            value.value<Qt::CheckState>()==Qt::Checked );
        return true;
    }
    return false;
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
  };

  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  ::disconnectAtEOL(
      layout,
      p.valueChanged.connect(
          [&p,checkBox]()
          {
              QSignalBlocker sb(checkBox);
              checkBox->setCheckState(p()?Qt::Checked:Qt::Unchecked);
          }
          )
      );

  return layout;
}
