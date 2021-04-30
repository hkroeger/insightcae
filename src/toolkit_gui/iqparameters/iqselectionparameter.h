#ifndef IQSELECTIONPARAMETER_H
#define IQSELECTIONPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/selectionparameter.h"

class IQSelectionParameter : public IQParameter
{
public:
  declareType(insight::SelectionParameter::typeName_());

  IQSelectionParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQSELECTIONPARAMETER_H
