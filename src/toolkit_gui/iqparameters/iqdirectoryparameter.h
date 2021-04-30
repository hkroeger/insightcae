#ifndef IQDIRECTORYPARAMETER_H
#define IQDIRECTORYPARAMETER_H

#include <iqparameter.h>

#include "base/parameters/pathparameter.h"

class IQDirectoryParameter : public IQParameter
{
public:
  declareType(insight::DirectoryParameter::typeName_());

  IQDirectoryParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQDIRECTORYPARAMETER_H
