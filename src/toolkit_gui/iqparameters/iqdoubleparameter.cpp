#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleValidator>

#include "iqdoubleparameter.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"

defineType(IQDoubleParameter);
addToFactoryTable(IQParameter, IQDoubleParameter);


IQDoubleParameter::IQDoubleParameter
(
    QObject* parent,
    IQParameterSetModel* psmodel,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
) : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{}

QString IQDoubleParameter::valueText() const
{
  return QString::number( dynamic_cast<const insight::DoubleParameter&>(parameter())() );
}

QVBoxLayout* IQDoubleParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto& p = dynamic_cast<insight::DoubleParameter&>(parameterRef());

  auto layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto lineEdit=new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &DoubleParameterWrapper::onDestruction);
//  le_->setText(QString::number(param()()));
  lineEdit->setValidator(new QDoubleValidator());
  lineEdit->setText(QString::number(p()));
  layout2->addWidget(lineEdit);

  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);


  auto applyFunction = [=]()
  {
      auto &p = dynamic_cast<insight::DoubleParameter&>(this->parameterRef());
      p.set(lineEdit->text().toDouble());
//      model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  ::disconnectAtEOL(
      layout,
      p.valueChanged.connect(
          [=]()
          {
              auto &p = dynamic_cast<const insight::DoubleParameter&>(parameter());
              QSignalBlocker sb(lineEdit);
              lineEdit->setText(QString::number(p()));
          }
          )
      );


  return layout;
}
