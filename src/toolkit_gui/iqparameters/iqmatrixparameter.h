#ifndef IQMATRIXPARAMETER_H
#define IQMATRIXPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/matrixparameter.h"

class IQMatrixParameter : public IQParameter
{
public:
  declareType(insight::MatrixParameter::typeName_());

  IQMatrixParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQMATRIXPARAMETER_H
