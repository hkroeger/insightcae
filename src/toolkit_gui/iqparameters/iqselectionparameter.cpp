#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

#include "iqselectionparameter.h"
#include "iqparametersetmodel.h"

defineType(IQSelectionParameter);
addToFactoryTable(IQParameter, IQSelectionParameter);

IQSelectionParameter::IQSelectionParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQSelectionParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::SelectionParameter&>(parameter());

  return QString::fromStdString( p.selection() );
}



QVBoxLayout* IQSelectionParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto& p = static_cast<const insight::SelectionParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
  QLabel *promptLabel = new QLabel("Selection:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto* selBox_=new QComboBox(editControlsContainer);
//  connect(selBox_, &QComboBox::destroyed, this, &SelectionParameterWrapper::onDestruction);
  for ( const std::string& s: p.items() )
  {
    selBox_->addItem(s.c_str());
  }
  selBox_->setCurrentIndex( p() );
  layout2->addWidget(selBox_);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  connect(apply, &QPushButton::pressed, [=]()
  {
      auto &p = dynamic_cast<insight::SelectionParameter&>(model->parameterRef(index));
      p.set(selBox_->currentIndex());
      model->notifyParameterChange(index);
  }
  );
  layout->addWidget(apply);


  return layout;
}
