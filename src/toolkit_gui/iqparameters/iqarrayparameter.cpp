#include <QHBoxLayout>
#include <QPushButton>


#include "iqarrayparameter.h"
#include "iqparametersetmodel.h"

defineType(IQArrayParameter);
addToFactoryTable(IQParameter, IQArrayParameter);

IQArrayParameter::IQArrayParameter
(
    QObject* parent,
    const QString& name,
    insight::Parameter& parameter,
    const insight::ParameterSet& defaultParameterSet
)
  : IQParameter(parent, name, parameter, defaultParameterSet)
{
}


QString IQArrayParameter::valueText() const
{
  auto& p = dynamic_cast<const insight::ArrayParameter&>(parameter());
  return QString( "array[%1]" ).arg(p.size());
}




QVBoxLayout* IQArrayParameter::populateEditControls(
        IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer)
{
  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);

  QHBoxLayout *layout2=new QHBoxLayout;

  QPushButton *addbtn=new QPushButton("+ Add new", editControlsContainer);
  layout2->addWidget(addbtn);

  auto *iqp = static_cast<IQParameter*>(index.internalPointer());
  auto mp = model->pathFromIndex(index);

  connect(addbtn, &QPushButton::clicked, iqp, [model,mp]()
  {
    auto rindex = model->indexFromPath(mp);
    Q_ASSERT(rindex.isValid());

    auto &p = dynamic_cast<insight::ArrayParameter&>(model->parameterRef(rindex));

    p.appendEmpty();

    int i=p.size()-1;
    model->beginInsertRows(rindex, std::max(0,i-1), i);
    auto* iqap = static_cast<IQParameter*>(rindex.internalPointer());
    iqap->append( model->decorateArrayElement(
          iqap,
          i,
          p[i],
          0 ) );
    model->endInsertRows();

    rindex = model->indexFromPath(mp);
    Q_ASSERT(rindex.isValid());

    model->notifyParameterChange(rindex);
  }
  );

  QPushButton *clearbtn=new QPushButton("Clear all", editControlsContainer);
  layout2->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked, iqp, [model,mp]()
  {
    auto rindex = model->indexFromPath(mp);
    Q_ASSERT(rindex.isValid());

    auto &p = dynamic_cast<insight::ArrayParameter&>(model->parameterRef(rindex));

    int n=p.size();
    model->beginRemoveRows(rindex, 0, n-1);
    auto *iqp = static_cast<IQParameter*>(rindex.internalPointer());
    for(auto* c: *iqp)
    {
      c->deleteLater();
    }
    iqp->clear();
    model->endRemoveRows();

    p.clear();

    rindex = model->indexFromPath(mp);
    Q_ASSERT(rindex.isValid());

    model->notifyParameterChange(rindex);
  }
  );

  layout->addLayout(layout2);

  return layout;
}
