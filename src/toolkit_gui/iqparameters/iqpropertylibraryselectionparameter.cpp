#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

#include "iqpropertylibraryselectionparameter.h"
#include "iqparametersetmodel.h"

defineType(IQPropertyLibrarySelectionParameter);
addToFactoryTable(IQParameter, IQPropertyLibrarySelectionParameter);

IQPropertyLibrarySelectionParameter::IQPropertyLibrarySelectionParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQPropertyLibrarySelectionParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::PropertyLibrarySelectionParameter&>(parameter());

  return QString::fromStdString( p.selection() );
}



QVBoxLayout* IQPropertyLibrarySelectionParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  const auto& p = static_cast<const insight::PropertyLibrarySelectionParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer);

  QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
  QLabel *promptLabel = new QLabel("Selection:", editControlsContainer);
  layout2->addWidget(promptLabel);
  auto* selBox_=new QComboBox(editControlsContainer);
//  connect(selBox_, &QComboBox::destroyed, this, &SelectionParameterWrapper::onDestruction);
  auto items = p.items();
  int i=0, seli=-1;
  for ( const auto& s: items )
  {
      if (s==p.selection())
      {
          seli=i;
      }
      selBox_->addItem(s.c_str());
      ++i;
  }
  insight::assertion(seli!=-1,
                     "selection not found in items");
  selBox_->setCurrentIndex( seli );
  layout2->addWidget(selBox_);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  connect(apply, &QPushButton::pressed, [=]()
  {
    auto &p = dynamic_cast<insight::PropertyLibrarySelectionParameter&>(model->parameterRef(index));
    p.setSelection(selBox_->currentText().toStdString());
    model->notifyParameterChange(index);
  }
  );
  layout->addWidget(apply);

  layout->addStretch();

  return layout;
}
