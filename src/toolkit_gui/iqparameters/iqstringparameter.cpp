#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>

#include "iqstringparameter.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"

defineType(IQStringParameter);
addToFactoryTable(IQParameter, IQStringParameter);

IQStringParameter::IQStringParameter
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


QString IQStringParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::StringParameter&>(parameter());

  return QString::fromStdString( p() );
}



QVBoxLayout* IQStringParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto *lineEdit=new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &StringParameterWrapper::onDestruction);
  lineEdit->setText(valueText());
  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
      auto &p = dynamic_cast<insight::StringParameter&>(this->parameterRef());
      p.set(lineEdit->text().toStdString());
//      model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  auto &p = dynamic_cast<insight::StringParameter&>(this->parameterRef());
  ::disconnectAtEOL(
      layout,
      p.valueChanged.connect(
          [=]()
          {
              auto &p = dynamic_cast<const insight::DoubleParameter&>(parameter());
              QSignalBlocker sb(lineEdit);
              lineEdit->setText(valueText());
          }
          )
      );

  return layout;
}
