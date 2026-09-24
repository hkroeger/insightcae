#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

#include "iqpropertylibraryselectionparameter.h"
#include "iqparametersetmodel.h"




defineTemplateType(IQSelectionParameterBase<insight::PropertyLibrarySelectionParameter>);
defineType(IQPropertyLibrarySelectionParameter);
addToFactoryTable(IQParameter, IQPropertyLibrarySelectionParameter);

addFunctionToStaticFunctionTable(
    IQHierarchicalDataGridViewDelegateEditorWidget, IQPropertyLibrarySelectionParameter,
    createDelegate,
    [](QObject* parent) { return new IQSelectionDelegate(parent); }
    );




IQPropertyLibrarySelectionParameter::IQPropertyLibrarySelectionParameter
(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
)
  : IQSelectionParameterBase<insight::PropertyLibrarySelectionParameter>(
          parent, hdmodel, element)
{}




QVariant IQPropertyLibrarySelectionParameter::value() const
{
    return QString::fromStdString(parameter().selection());
}
