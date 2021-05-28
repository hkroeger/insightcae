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
  auto* selBox=new QComboBox(editControlsContainer);
  for ( auto& pair: p.items() )
  {
    selBox->addItem( QString::fromStdString(pair.first) );
  }
  selBox->setCurrentIndex(
        selBox->findText(
          QString::fromStdString(p.selection())
          )
        );

  layout2->addWidget(selBox);
  layout->addLayout(layout2);


  QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
  layout->addWidget(apply);

  layout->addStretch();



  auto* iqp = static_cast<IQParameter*>(index.internalPointer());

  connect(apply, &QPushButton::clicked, iqp, [=]()
  {
    auto &p = dynamic_cast<insight::SelectableSubsetParameter&>(model->parameterRef(index));

//    qDebug()<<"beginRemoveRows"<<index;
//    qDebug()<<iqp;
//    qDebug()<<iqp->size();
    if (iqp->size())
    {
      // remove existing child params
      model->beginRemoveRows(index, 0, iqp->size()-1);
      for (auto* c: *iqp)
      {
  //      qDebug()<<"delete "<<c->name();
        c->deleteLater();
      }
      iqp->clear();
      model->endRemoveRows();
    }

    // change data
    p.selection() = selBox->currentText().toStdString();

    if (p().size())
    {
      // repopulate
      model->beginInsertRows(index, 0, p().size()-1);
      auto newc = model->decorateSubdictContent(iqp, p(), 0);
      model->endInsertRows();
    }

    model->notifyParameterChange(index);
  }
  );

  return layout;
}
