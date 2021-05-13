#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

#include "iqselectablesubsetparameter.h"
#include "iqparametersetmodel.h"

defineType(IQSelectableSubsetParameter);
addToFactoryTable(IQParameter, IQSelectableSubsetParameter);

IQSelectableSubsetParameter::IQSelectableSubsetParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQSelectableSubsetParameter::valueText() const
{
  const auto& p = dynamic_cast<const insight::SelectableSubsetParameter&>(parameter());

  return QString::fromStdString( p.selection() );
}



QVBoxLayout* IQSelectableSubsetParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  const auto&p = dynamic_cast<const insight::SelectableSubsetParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer);

  QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);
  layout2->addWidget(new QLabel("Selection:", editControlsContainer));
  auto* selBox_=new QComboBox(editControlsContainer);
  for ( auto& pair: p.items() )
  {
    selBox_->addItem( QString::fromStdString(pair.first) );
  }
  selBox_->setCurrentIndex(
        selBox_->findText(
          QString::fromStdString(p.selection())
          )
        );

  layout2->addWidget(selBox_);
  layout->addLayout(layout2);


  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);

  layout->addStretch();




  connect(apply, &QPushButton::clicked, [=]()
  {
    auto* iqp = static_cast<IQParameter*>(index.internalPointer());
    auto &p = dynamic_cast<insight::SelectableSubsetParameter&>(model->parameterRef(index));

    // remove existing child params
    model->beginRemoveRows(index, 0, iqp->size()-1);
    for (auto* c: *iqp)
    {
      c->deleteLater();
    }
    iqp->clear();
    model->endRemoveRows();

    // change data
    p.selection() = selBox_->currentText().toStdString();

    // repopulate
    model->beginInsertRows(index, 0, p().size()-1);
    auto newc = model->decorateSubdictContent(iqp, p(), 0);
    model->endInsertRows();

    model->notifyParameterChange(index);
  }
  );

  return layout;
}
