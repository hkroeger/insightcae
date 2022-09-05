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



QVBoxLayout* IQSelectableSubsetParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  const auto&p = dynamic_cast<const insight::SelectableSubsetParameter&>(parameter());

  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;
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
  auto mp = model->pathFromIndex(index);

  connect(apply, &QPushButton::clicked, iqp, [iqp,model,selBox,mp]()
  {
    auto rindex = model->indexFromPath(mp);
    Q_ASSERT(rindex.isValid());

    auto* param = dynamic_cast<insight::SelectableSubsetParameter*>(&(model->parameterRef(rindex)));

    if (iqp->size())
    {
      // remove existing child params
      model->beginRemoveRows(rindex, 0, iqp->size()-1);
      for (auto* c: *iqp)
      {
        c->deleteLater();
      }
      iqp->clear();
      model->endRemoveRows();
    }

    // index invalid after model structure change
    rindex = model->indexFromPath(mp);
    Q_ASSERT(rindex.isValid());

    // change data
    param->selection() = selBox->currentText().toStdString();

    if ((*param)().size())
    {
      // repopulate
      model->beginInsertRows(rindex, 0, (*param)().size()-1);
      auto newc = model->decorateSubdictContent(iqp, (*param)(), 0);
      model->endInsertRows();
    }

    model->notifyParameterChange(rindex);
  }
  );

  return layout;
}
