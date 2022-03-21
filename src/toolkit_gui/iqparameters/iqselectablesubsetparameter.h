#ifndef IQSELECTABLESUBSETPARAMETER_H
#define IQSELECTABLESUBSETPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/selectablesubsetparameter.h"

class TOOLKIT_GUI_EXPORT IQSelectableSubsetParameter : public IQParameter
{
public:
  declareType(insight::SelectableSubsetParameter::typeName_());

  IQSelectableSubsetParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};

#endif // IQSELECTABLESUBSETPARAMETER_H
