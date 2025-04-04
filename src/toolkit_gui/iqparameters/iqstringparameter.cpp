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
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQSpecializedParameter<insight::StringParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{
}


QString IQStringParameter::valueText() const
{
  return QString::fromStdString( parameter()() );
}

bool IQStringParameter::setValue(QVariant value)
{
    parameterRef().set(value.toString().toStdString());
    return true;
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
      parameterRef().set(lineEdit->text().toStdString());
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  ::disconnectAtEOL(
      layout,
      parameterRef().valueChanged.connect(
          [=]()
          {
              QSignalBlocker sb(lineEdit);
              lineEdit->setText(valueText());
          }
          )
      );

  return layout;
}
