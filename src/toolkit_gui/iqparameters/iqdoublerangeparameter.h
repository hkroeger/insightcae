#ifndef IQDOUBLERANGEPARAMETER_H
#define IQDOUBLERANGEPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/doublerangeparameter.h"

class IQDoubleRangeParameter : public IQParameter
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
