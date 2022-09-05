#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QIntValidator>

#include "iqintparameter.h"
#include "iqparametersetmodel.h"


defineType(IQIntParameter);
addToFactoryTable(IQParameter, IQIntParameter);

IQIntParameter::IQIntParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQIntParameter::valueText() const
{
  return QString::number( dynamic_cast<const insight::IntParameter&>(parameter())() );
}


QVBoxLayout* IQIntParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Value:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto* lineEdit=new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &IntParameterWrapper::onDestruction);
  lineEdit->setText(QString::number( dynamic_cast<const insight::IntParameter&>(parameter())() ));
  lineEdit->setValidator(new QIntValidator());
  layout2->addWidget(lineEdit);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);

  layout->addStretch();

  auto applyFunction = [=]()
  {
    auto &p = dynamic_cast<insight::IntParameter&>(model->parameterRef(index));
    p()=lineEdit->text().toInt();
    model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  return layout;
}
