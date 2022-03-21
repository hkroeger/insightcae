#ifndef IQDOUBLERANGEPARAMETER_H
#define IQDOUBLERANGEPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/doublerangeparameter.h"

class TOOLKIT_GUI_EXPORT IQDoubleRangeParameter : public IQParameter
{
public:
    declareType(insight::DoubleRangeParameter::typeName_());

    IQDoubleRangeParameter
    (
        QObject* parent,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    QString valueText() const override;

    QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQDOUBLERANGEPARAMETER_H
