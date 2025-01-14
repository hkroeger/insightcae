#ifndef IQDOUBLERANGEPARAMETER_H
#define IQDOUBLERANGEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/doublerangeparameter.h"

class TOOLKIT_GUI_EXPORT IQDoubleRangeParameter
    : public IQSpecializedParameter<insight::DoubleRangeParameter>
{
public:
    declareType(insight::DoubleRangeParameter::typeName_());

    IQDoubleRangeParameter
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

#endif // IQDOUBLERANGEPARAMETER_H
