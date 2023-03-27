#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>

#include "iqstringparameter.h"
#include "iqparametersetmodel.h"

defineType(IQStringParameter);
addToFactoryTable(IQParameter, IQStringParameter);

IQStringParameter::IQStringParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQStringParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::StringParameter&>(parameter());

  return QString::fromStdString( p() );
}



QVBoxLayout* IQStringParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

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
    auto &p = dynamic_cast<insight::StringParameter&>(model->parameterRef(index));
    p()=lineEdit->text().toStdString();
    model->notifyParameterChange(index);
  };

  connect(lineEdit, &QLineEdit::returnPressed, applyFunction);
  connect(apply, &QPushButton::pressed, applyFunction);

  return layout;
}
