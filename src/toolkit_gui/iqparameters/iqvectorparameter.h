#ifndef IQVECTORPARAMETER_H
#define IQVECTORPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class IQVectorParameter : public IQParameter
{
public:
  declareType(insight::VectorParameter::typeName_());

  IQVectorParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQVECTORPARAMETER_H
