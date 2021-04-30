#ifndef IQSTRINGPARAMETER_H
#define IQSTRINGPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class IQStringParameter : public IQParameter
{
public:
  declareType(insight::StringParameter::typeName_());

  IQStringParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQSTRINGPARAMETER_H
