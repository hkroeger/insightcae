#ifndef IQBOOLPARAMETER_H
#define IQBOOLPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class IQBoolParameter : public IQParameter
{
public:
  declareType(insight::BoolParameter::typeName_());

  IQBoolParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQBOOLPARAMETER_H
