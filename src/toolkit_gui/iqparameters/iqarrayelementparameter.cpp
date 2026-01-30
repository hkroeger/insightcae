#include <cstdio>
#include <cstring>

#include "iqarrayelementparameter.h"
#include "base/parameters/arrayparameter.h"
#include "cadgeometryparameter.h"
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
#include "iqsubsetparameter.h"
#include "iqselectablesubsetparameter.h"
#include "iqdoublerangeparameter.h"
#include "iqspatialtransformationparameter.h"
#include "iqparametersetmodel.h"
#include "iqcadgeometryparameter.h"



defineType(IQArrayElementParameterBase);




defineFactoryTable
(
    IQArrayElementParameterBase,
      LIST(
        QObject* parent,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element ),
      LIST(parent, hdmodel, element)
);




IQArrayElementParameterBase::IQArrayElementParameterBase(
    QObject *,
    IQHierarchicalDataModel*,
    insight::hierarchicalData::Element *)
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
createIQArrayElement(IQSubsetParameter, "subset");
createIQArrayElement(IQSelectableSubsetParameter, "selectableSubset");
createIQArrayElement(IQDoubleRangeParameter, "doubleRange");
createIQArrayElement(IQSpatialTransformationParameter, "spatialTransformation");
createIQArrayElement(IQCADGeometryParameter, "cadgeometry");




IQHierarchicalDataElement *IQArrayParameter::createForChild(
    IQHierarchicalDataModel *model,
    insight::hierarchicalData::Element *ce )
{
    IQHierarchicalDataElement *ne{ nullptr };

    if (IQArrayElementParameterBase::has_factory(ce->type()))
    {
        ne =  dynamic_cast<IQParameter*>(
            IQArrayElementParameterBase::lookup(
                ce->type(), this, model, ce)
            );
    }
    else
    {
        ne = dynamic_cast<IQParameter*>(
            new IQParameterArrayElement(
                this, model, ce)
            );
    }

    ne->connectSignals();

    return ne;
}






template<class IQBaseParameter, const char *N>
void IQArrayElementParameter<IQBaseParameter, N>::deleteFromArray()
{
    auto *containingArray =
        dynamic_cast<IQArrayParameter*>(
            this->parentElement());

    auto& array_param =
        containingArray->parameterRef(); // array element must have a parent (the actual array)

    auto *m=this->model();
    auto myIndex = m->indexOfElement(**this, 0);
    int row = myIndex.row();

    // this->removeFromViews(false);

    array_param.eraseValue(row);
    // this->deleteLater();
}




template<class IQBaseParameter, const char *N>
void IQArrayElementParameter<IQBaseParameter, N>
    ::populateContextMenu(QMenu *cm, IQCADModel3DViewer *)
{
  auto *removeAction = new QAction("Remove this array element");
  cm->addAction(removeAction);

  QObject::connect(removeAction, &QAction::triggered,
                   this, &IQArrayElementParameter::deleteFromArray);
}
