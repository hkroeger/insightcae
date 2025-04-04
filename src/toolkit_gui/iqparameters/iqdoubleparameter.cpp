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
    insight::Parameter* parameter,
    const insight::ParameterSet& defaultParameterSet
)
    : IQSpecializedParameter<insight::DoubleParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{}

QString IQDoubleParameter::valueText() const
{
  return QString::number( parameter()() );
}

bool IQDoubleParameter::setValue(QVariant value)
{
    if (value.canConvert<double>())
    {
        bool ok;
        parameterRef().set(value.toDouble(&ok));
        return ok;
    }
    return false;
}

QVBoxLayout* IQDoubleParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto& p = parameterRef();

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
      auto &p = parameterRef();
      p.set(lineEdit->text().toDouble());
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  // handle external value change
  ::disconnectAtEOL(
      layout,
      p.valueChanged.connect(
          [=]()
          {
              QSignalBlocker sb(lineEdit);
              lineEdit->setText(QString::number(parameter()()));
          }
          )
      );


  return layout;
}
