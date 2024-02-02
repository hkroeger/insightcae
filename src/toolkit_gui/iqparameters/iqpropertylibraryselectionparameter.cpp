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
    IQParameterSetModel* psmodel,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{
}


QString IQPropertyLibrarySelectionParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::PropertyLibrarySelectionParameter&>(parameter());

  return QString::fromStdString( p.selection() );
}



QVBoxLayout* IQPropertyLibrarySelectionParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto& p = static_cast<const insight::PropertyLibrarySelectionParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
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

      if (auto *pl=p.propertyLibrary())
      {
          auto iconp=pl->icon(s);
          selBox_->addItem(QPixmap(QString::fromStdString(iconp)), s.c_str());
      }
      else
      {
          selBox_->addItem(s.c_str());
      }

      ++i;
  }
  insight::assertion(seli!=-1,
                     "selection not found in items");
  selBox_->setIconSize(QSize(200,150));
  selBox_->setCurrentIndex( seli );
  layout2->addWidget(selBox_);
  layout->addLayout(layout2);

  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  connect(apply, &QPushButton::pressed, [=]()
  {
    auto &p = dynamic_cast<insight::PropertyLibrarySelectionParameter&>(this->parameterRef());
    p.setSelection(selBox_->currentText().toStdString());
//    model->notifyParameterChange(index);
  }
  );
  layout->addWidget(apply);


  return layout;
}
