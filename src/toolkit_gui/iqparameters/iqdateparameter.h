#ifndef IQDATEPARAMETER_H
#define IQDATEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQDateParameter : public IQParameter
{
public:
    declareType(insight::DateParameter::typeName_());

    IQDateParameter
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

#endif // IQDATEPARAMETER_H
