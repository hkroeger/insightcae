#ifndef IQDATETIMEPARAMETER_H
#define IQDATETIMEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQDateTimeParameter : public IQParameter
{
public:
    declareType(insight::DateTimeParameter::typeName_());

    IQDateTimeParameter
        (
            QObject* parent,
            IQParameterSetModel* psmodel,
            const QString& name,
            insight::Parameter& parameter,
            const insight::ParameterSet& defaultParameterSet
            );

    QString valueText() const override;

    QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer) override;
};

#endif // IQDATETIMEPARAMETER_H
