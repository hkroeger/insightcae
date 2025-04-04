#ifndef IQLABELEDARRAYPARAMETER_H
#define IQLABELEDARRAYPARAMETER_H

#include "toolkit_gui_export.h"
#include "iqparameter.h"
#include "base/parameters/labeledarrayparameter.h"

class TOOLKIT_GUI_EXPORT IQLabeledArrayParameter
    : public IQSpecializedParameter<insight::LabeledArrayParameter>
{
public:
    declareType(insight::LabeledArrayParameter::typeName_());

    IQLabeledArrayParameter
        (
            QObject* parent,
            IQParameterSetModel* psmodel,
            insight::Parameter* parameter,
            const insight::ParameterSet& defaultParameterSet
            );

    QString valueText() const override;


    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer ) override;
};

#endif // IQLABELEDARRAYPARAMETER_H
