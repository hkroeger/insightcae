#ifndef IQPATHPARAMETER_H
#define IQPATHPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/pathparameter.h"

class TOOLKIT_GUI_EXPORT IQPathParameter : public IQParameter
{
public:
  declareType(insight::PathParameter::typeName_());

  IQPathParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQPATHPARAMETER_H
