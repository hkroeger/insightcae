#include <cstdio>
#include <cstring>

#include "base/hierarchicalelement.h"
#include "base/translations.h"

#include "iqlabeledarrayelementparameter.h"

#include "iqintparameter.h"
#include "iqdoubleparameter.h"
#include "iqboolparameter.h"
#include "iqvectorparameter.h"
#include "iqstringparameter.h"
#include "iqpathparameter.h"
#include "iqsubsetparameter.h"
#include "iqdirectoryparameter.h"

#include "iqlabeledarrayparameter.h"
#include "iqarrayparameter.h"
#include "iqmatrixparameter.h"
#include "iqselectionparameter.h"
#include "iqselectablesubsetparameter.h"
#include "iqdoublerangeparameter.h"
#include "iqspatialtransformationparameter.h"

#include "iqparametersetmodel.h"

#include <QInputDialog>


defineType(IQLabeledArrayElementParameterBase);

defineFactoryTable
    (
        IQLabeledArrayElementParameterBase,
        LIST(
            QObject* parent,
            IQHierarchicalDataModel* psmodel,
            insight::hierarchicalData::Element* parameter ),
        LIST(parent, psmodel, parameter)
        );



IQLabeledArrayElementParameterBase::IQLabeledArrayElementParameterBase(
    QObject *,
    IQHierarchicalDataModel*,
    insight::hierarchicalData::Element *)
{}

#define createIQLabeledArrayElement(BaseT, NAME) \
const char BaseT##LabeledArrayElementName[] = NAME; \
    typedef IQLabeledArrayElementParameter<BaseT,BaseT##LabeledArrayElementName> BaseT##LabeledArrayElement; \
    defineTemplateType(BaseT##LabeledArrayElement); \
    addToFactoryTable(IQLabeledArrayElementParameterBase, BaseT##LabeledArrayElement)


createIQLabeledArrayElement(IQParameter, "generic");
createIQLabeledArrayElement(IQIntParameter, "int");
createIQLabeledArrayElement(IQDoubleParameter, "double");
createIQLabeledArrayElement(IQBoolParameter, "bool");
createIQLabeledArrayElement(IQVectorParameter, "vector");
createIQLabeledArrayElement(IQStringParameter, "string");
createIQLabeledArrayElement(IQPathParameter, "path");
createIQLabeledArrayElement(IQDirectoryParameter, "directory");
createIQLabeledArrayElement(IQArrayParameter, "array");
createIQLabeledArrayElement(IQMatrixParameter, "matrix");
createIQLabeledArrayElement(IQSelectionParameter, "selection");
createIQLabeledArrayElement(IQSubsetParameter, "subset");
createIQLabeledArrayElement(IQSelectableSubsetParameter, "selectableSubset");
createIQLabeledArrayElement(IQDoubleRangeParameter, "doubleRange");
createIQLabeledArrayElement(IQSpatialTransformationParameter, "spatialTransformation");





IQHierarchicalDataElement *IQLabeledArrayParameter::createForChild(
    IQHierarchicalDataModel *model, insight::hierarchicalData::Element *p)
{
    IQParameter *np;
    if (IQLabeledArrayElementParameterBase::has_factory(p->type()))
    {
        np=dynamic_cast<IQParameter*>(
            IQLabeledArrayElementParameterBase::lookup(p->type(), this, model, p)
            );
    }
    else
    {
        np=dynamic_cast<IQParameter*>(
            new IQParameterLabeledArrayElement(this, model, p)
            );
    }

    np->connectSignals();

    return np;
}

template<class IQBaseParameter, const char *N>
void IQLabeledArrayElementParameter<IQBaseParameter, N>
    ::populateContextMenu(
        QMenu *cm, IQCADModel3DViewer *viewer)
{
    if (!arrayParameter()->parameter().keysAreLocked())
    {
        auto *removeAction = new QAction("Remove this array element");
        cm->addAction(removeAction);

        QObject::connect(
            removeAction, &QAction::triggered, this,
             [this]()
             {

                 auto *array = dynamic_cast<IQLabeledArrayParameter*>(this->parentElement());
                 auto row = array->children().indexOf(this);

                 auto &arrayp = dynamic_cast<insight::LabeledArrayParameter&>(array->parameterRef());
                 arrayp.eraseValue(arrayp.childElementName(row));
             }
             );
    }

    if (!arrayParameter()->parameter().keysAreLocked())
    {
        auto *renameAction = new QAction("Change label of element");
        cm->addAction(renameAction);

        QObject::connect(
            renameAction, &QAction::triggered, this,
             [this,cm]()
             {
                 auto *iqp = dynamic_cast<IQLabeledArrayParameter*>(this->parentElement());
                 auto row = iqp->children().indexOf(this);
                 auto &p = dynamic_cast<insight::LabeledArrayParameter&>(iqp->parameterRef());
                 auto label = p.childElementName(row);
                 auto newl = QInputDialog::getText(
                     cm->parentWidget(),
                     _("Change Item Label"), _("New label:"),
                     QLineEdit::Normal, QString::fromStdString(label) );
                 if (!newl.isEmpty())
                 {
                     p.changeLabel(label, newl.toStdString());
                 }
             }
             );
    }

}
