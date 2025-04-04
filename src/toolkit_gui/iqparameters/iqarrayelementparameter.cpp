#include <cstdio>
#include <cstring>

#include "iqarrayelementparameter.h"
#include "base/parameters/arrayparameter.h"
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
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet ),
      LIST(parent, psmodel, parameter, defaultParameterSet)
);




IQArrayElementParameterBase::IQArrayElementParameterBase(
    QObject *,
    IQParameterSetModel*,
    insight::Parameter *,
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
    insight::Parameter *p,
    const insight::ParameterSet &defaultParameterSet )
{
  IQParameter *np;
  if (IQArrayElementParameterBase::has_factory(p->type()))
  {
    np=dynamic_cast<IQParameter*>(
        IQArrayElementParameterBase::lookup(p->type(), parent, psmodel, p, defaultParameterSet)
        );
  }
  else
  {
    np=dynamic_cast<IQParameter*>(
        new IQParameterArrayElement(parent, psmodel, p, defaultParameterSet)
        );
  }

  np->connectSignals();

  return np;
}




template<class IQBaseParameter, const char *N>
void IQArrayElementParameter<IQBaseParameter, N>::deleteFromArray()
{
    auto *containingArray =
        dynamic_cast<IQArrayParameter*>(
            this->parentParameter());

    auto& array_param =
        containingArray->parameterRef(); // array element must have a parent (the actual array)

    auto *m=this->model();
    auto myIndex = m->indexFromParameter(**this, 0);
    int row = myIndex.row();

    // this->removeFromViews(false);

    array_param.eraseValue(row);
    // this->deleteLater();
}




template<class IQBaseParameter, const char *N>
void IQArrayElementParameter<IQBaseParameter, N>::populateContextMenu(QMenu *cm)
{
  auto *removeAction = new QAction("Remove this array element");
  cm->addAction(removeAction);

  QObject::connect(removeAction, &QAction::triggered,
                   this, &IQArrayElementParameter::deleteFromArray);
}
