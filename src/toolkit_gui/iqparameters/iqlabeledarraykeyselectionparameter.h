#ifndef IQLABELEDARRAYKEYSELECTIONPARAMETER_H
#define IQLABELEDARRAYKEYSELECTIONPARAMETER_H

#include "toolkit_gui_export.h"
#include "iqselectionparameter.h"
#include "base/parameters/labeledarraykeyselectionparameter.h"

typedef IQSelectionParameterBase<insight::LabeledArrayKeySelectionParameter>
    IQLabeledArrayKeySelectionParameterBase;

class TOOLKIT_GUI_EXPORT IQLabeledArrayKeySelectionParameter
    : public IQLabeledArrayKeySelectionParameterBase
{
public:
    declareType(insight::LabeledArrayKeySelectionParameter::typeName_());

    IQLabeledArrayKeySelectionParameter(
        QObject* parent,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element);

    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer* viewer) override;
};

#endif // IQLABELEDARRAYKEYSELECTIONPARAMETER_H
