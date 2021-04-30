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




QVBoxLayout* IQArrayParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer);

  QHBoxLayout *layout2=new QHBoxLayout(editControlsContainer);

  QPushButton *addbtn=new QPushButton("+ Add new", editControlsContainer);
  layout2->addWidget(addbtn);
  connect(addbtn, &QPushButton::clicked, [=]()
  {
    auto &p = dynamic_cast<insight::ArrayParameter&>(model->parameterRef(index));

    p.appendEmpty();

    int i=p.size()-1;
    model->beginInsertRows(index, i-1, i);
    auto* iqap = static_cast<IQParameter*>(index.internalPointer());
    iqap->append( model->decorateArrayElement(
          iqap,
          i,
          p[i],
          0 ) );
    model->endInsertRows();

    model->notifyParameterChange(index);
  }
  );

  QPushButton *clearbtn=new QPushButton("Clear all", editControlsContainer);
  layout2->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked, [=]()
  {
    auto &p = dynamic_cast<insight::ArrayParameter&>(model->parameterRef(index));


    int n=p.size();
    model->beginRemoveRows(index, 0, n-1);
    auto *iqp = static_cast<IQParameter*>(index.internalPointer());
    for(auto* c: *iqp)
    {
      c->deleteLater();
    }
    iqp->clear();
    model->endRemoveRows();

    p.clear();

    model->notifyParameterChange(index);
  }
  );

  layout->addLayout(layout2);
  layout->addStretch();

  return layout;
}
