#ifndef IQDIRECTORYPARAMETER_H
#define IQDIRECTORYPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/pathparameter.h"

class TOOLKIT_GUI_EXPORT IQDirectoryParameter : public IQParameter
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

  QVBoxLayout* populateEditControls(
          IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

};

#endif // IQDIRECTORYPARAMETER_H
