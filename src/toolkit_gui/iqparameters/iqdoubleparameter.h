#ifndef IQDOUBLEPARAMETER_H
#define IQDOUBLEPARAMETER_H

#include <iqparameter.h>
#include "base/parameters/simpleparameter.h"

class IQDoubleParameter : public IQParameter
{
public:
    declareType(insight::DoubleParameter::typeName_());

    IQDoubleParameter
    (
        QObject* parent,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    QString valueText() const override;

    QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQDOUBLEPARAMETER_H
