#ifndef IQPROPERTYLIBRARYSELECTIONPARAMETER_H
#define IQPROPERTYLIBRARYSELECTIONPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/propertylibraryselectionparameter.h"

class TOOLKIT_GUI_EXPORT IQPropertyLibrarySelectionParameter : public IQParameter
{
public:
  declareType(insight::PropertyLibrarySelectionParameter::typeName_());

  IQPropertyLibrarySelectionParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer) override;

};
#endif // IQPROPERTYLIBRARYSELECTIONPARAMETER_H
