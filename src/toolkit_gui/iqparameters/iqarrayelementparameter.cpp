#include <cstdio>
#include <cstring>


#include "iqarrayelementparameter.h"

#include "iqintparameter.h"
#include "iqdoubleparameter.h"
#include "iqboolparameter.h"
#include "iqvectorparameter.h"
#include "iqstringparameter.h"
#include "iqpathparameter.h"
#include "iqdirectoryparameter.h"

#include "iqarrayparameter.h"
#include "iqmatrixparameter.h"
#include "iqselectionparameter.h"
#include "iqselectablesubsetparameter.h"
#include "iqdoublerangeparameter.h"

#include "iqparametersetmodel.h"


defineType(IQArrayElementParameterBase);

defineFactoryTable
(
    IQArrayElementParameterBase,
      LIST(QObject* parent, const QString& name, insight::Parameter& parameter, const insight::ParameterSet& defaultParameterSet),
      LIST(parent, name, parameter, defaultParameterSet)
);



IQArrayElementParameterBase::IQArrayElementParameterBase(
    QObject *,
    const QString &,
    insight::Parameter &,
    const insight::ParameterSet &)
{}

#define createIQArrayElement(BaseT, NAME) \
const char BaseT##ArrayElementName[] = NAME; \
typedef IQArrayElementParameter<BaseT,BaseT##ArrayElementName> BaseT##ArrayElement; \
defineTemplateType(BaseT##ArrayElement); \
addToFactoryTable(IQArrayElementParameterBase, BaseT##ArrayElement)


createIQArrayElement(IQParameter, "generic");
createIQArrayElement(IQIntParameter, "int");
createIQArrayElement(IQDoubleParameter, "double");
createIQArrayElement(IQBoolParameter, "bool");
createIQArrayElement(IQVectorParameter, "vector");
createIQArrayElement(IQStringParameter, "string");
createIQArrayElement(IQPathParameter, "path");
createIQArrayElement(IQDirectoryParameter, "directory");
createIQArrayElement(IQArrayParameter, "array");
createIQArrayElement(IQMatrixParameter, "matrix");
createIQArrayElement(IQSelectionParameter, "selection");
createIQArrayElement(IQSelectableSubsetParameter, "selectablesubset");
createIQArrayElement(IQDoubleRangeParameter, "doubleRange");


IQParameter *IQArrayElementParameterBase::create(QObject *parent, const QString &name, insight::Parameter &p, const insight::ParameterSet &defaultParameterSet)
{
  try {
    return dynamic_cast<IQParameter*>(
        IQArrayElementParameterBase::lookup(p.type(), parent, name, p, defaultParameterSet)
        );
  }
  catch (...)
  {
    return dynamic_cast<IQParameter*>(
        new IQParameterArrayElement(parent, name, p, defaultParameterSet)
        );
  }
}




template<class IQBaseParameter, const char *N>
void IQArrayElementParameter<IQBaseParameter, N>::populateContextMenu(IQParameterSetModel* model, const QModelIndex &index, QMenu *cm)
{
  auto *removeAction = new QAction("Remove this array element");
  cm->addAction(removeAction);

  auto* iqp = static_cast<IQParameter*>(index.internalPointer());
  auto mp = model->pathFromIndex(model->parent(index));
  auto row = index.row();

  QObject::connect(removeAction, &QAction::triggered, iqp,
          [model,mp,row,iqp]()
          {
            auto parentIndex = model->indexFromPath(mp);
            Q_ASSERT(parentIndex.isValid());

            auto &parentParameter = dynamic_cast<insight::ArrayParameter&>(model->parameterRef(parentIndex));

            model->beginRemoveRows(parentIndex, row, row);

            iqp->deleteLater();

            auto *aiqp = static_cast<IQParameter*>(parentIndex.internalPointer());
            aiqp->erase(aiqp->begin()+row);

            model->endRemoveRows();


            parentParameter.eraseValue(row);

            parentIndex = model->indexFromPath(mp);
            Q_ASSERT(parentIndex.isValid());
            model->notifyParameterChange(parentIndex);

            // change name for all subsequent parameters
            for (int i=row; i<aiqp->size(); ++i)
            {
              (*aiqp)[i]->setName(QString("%1").arg(i));
              model->notifyParameterChange( model->index(i, 1, parentIndex) );
            }
          }
  );
}
