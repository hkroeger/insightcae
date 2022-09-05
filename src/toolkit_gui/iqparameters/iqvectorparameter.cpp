#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "iqvectorparameter.h"
#include "iqparametersetmodel.h"

defineType(IQVectorParameter);
addToFactoryTable(IQParameter, IQVectorParameter);

IQVectorParameter::IQVectorParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


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
  auto *le_=new QLineEdit(editControlsContainer);
//  connect(le_, &QLineEdit::destroyed, this, &VectorParameterWrapper::onDestruction);
  le_->setText( QString::fromStdString(insight::valueToString(p())) );
  layout2->addWidget(le_);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);

  layout->addStretch();

  auto applyFunction = [=]()
  {
    auto& p = dynamic_cast<insight::VectorParameter&>(model->parameterRef(index));
    insight::stringToValue(le_->text().toStdString(), p());
    model->notifyParameterChange(index);
  };

  connect(le_, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  return layout;
}
