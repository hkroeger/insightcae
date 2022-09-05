#ifndef IQSELECTIONPARAMETER_H
#define IQSELECTIONPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/selectionparameter.h"

class TOOLKIT_GUI_EXPORT IQSelectionParameter : public IQParameter
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

  QVBoxLayout* populateEditControls(
          IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

};

#endif // IQSELECTIONPARAMETER_H
