#ifndef IQINTPARAMETER_H
#define IQINTPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"


class IQIntParameter : public IQParameter
{
public:
    declareType(insight::IntParameter::typeName_());

    IQIntParameter
    (
        QObject* parent,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
    );

    QString valueText() const override;

    QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;
};

#endif // IQINTPARAMETER_H
