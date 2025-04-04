#ifndef IQINTPARAMETER_H
#define IQINTPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"


class TOOLKIT_GUI_EXPORT IQIntParameter
    : public IQSpecializedParameter<insight::IntParameter>
{
public:
    declareType(insight::IntParameter::typeName_());

    IQIntParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    QString valueText() const override;
    bool setValue(QVariant value) override;

    QVBoxLayout* populateEditControls(
            QWidget* editControlsContainer,
            IQCADModel3DViewer *viewer) override;
};

#endif // IQINTPARAMETER_H
