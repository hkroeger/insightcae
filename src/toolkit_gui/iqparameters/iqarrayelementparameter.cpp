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
#include "iqspatialtransformationparameter.h"

#include "iqparametersetmodel.h"


defineType(IQArrayElementParameterBase);

defineFactoryTable
(
    IQArrayElementParameterBase,
      LIST(
        QObject* parent,
        IQParameterSetModel* psmodel,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet ),
      LIST(parent, psmodel, name, parameter, defaultParameterSet)
);



IQArrayElementParameterBase::IQArrayElementParameterBase(
    QObject *,
    IQParameterSetModel*,
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
createIQArrayElement(IQSelectableSubsetParameter, "selectableSubset");
createIQArrayElement(IQDoubleRangeParameter, "doubleRange");
createIQArrayElement(IQSpatialTransformationParameter, "spatialTransformation");


IQParameter *IQArrayElementParameterBase::create(
    QObject *parent,
    IQParameterSetModel* psmodel,
    const QString &name,
    insight::Parameter &p,
    const insight::ParameterSet &defaultParameterSet )
{
  IQParameter *np;
  if (IQArrayElementParameterBase::has_factory(p.type()))
  {
    np=dynamic_cast<IQParameter*>(
        IQArrayElementParameterBase::lookup(p.type(), parent, psmodel, name, p, defaultParameterSet)
        );
  }
  else
  {
    np=dynamic_cast<IQParameter*>(
        new IQParameterArrayElement(parent, psmodel, name, p, defaultParameterSet)
        );
  }

  // connect outside constructor because of virtual "path" function is involved
  np->updateConnection = p.valueChanged.connect(
      [psmodel,np]() { psmodel->notifyParameterChange(np->path().toStdString(), true); }
      //std::bind(&IQParameterSetModel::notifyParameterChange, psmodel, np->path().toStdString(), true)
      );

  return np;
}




template<class IQBaseParameter, const char *N>
void IQArrayElementParameter<IQBaseParameter, N>::populateContextMenu(QMenu *cm)
{
  auto *removeAction = new QAction("Remove this array element");
  cm->addAction(removeAction);

//  auto row = index.row();

  QObject::connect(removeAction, &QAction::triggered, this,
          [this]()
          {

            auto *array = dynamic_cast<IQArrayParameter*>(this->parentParameter());
            auto row = array->children().indexOf(this);

            auto &arrayp = dynamic_cast<insight::ArrayParameter&>(array->parameterRef());
            arrayp.eraseValue(row);

            //model->removeArrayElement(model->index(row, 0, parentIndex));

//            auto &parentParameter = dynamic_cast<insight::ArrayParameter&>(model->parameterRef(parentIndex));

//            model->beginRemoveRows(parentIndex, row, row);

//            iqp->deleteLater();

//            auto *aiqp = static_cast<IQParameter*>(parentIndex.internalPointer());
//            aiqp->erase(aiqp->begin()+row);

//            model->endRemoveRows();


//            parentParameter.eraseValue(row);

//            parentIndex = model->indexFromPath(mp);
//            Q_ASSERT(parentIndex.isValid());
//            model->notifyParameterChange(parentIndex);

//            // change name for all subsequent parameters
//            for (int i=row; i<aiqp->size(); ++i)
//            {
//              (*aiqp)[i]->setName(QString("%1").arg(i));
//              model->notifyParameterChange( model->index(i, 1, parentIndex) );
//            }
          }
  );
}
