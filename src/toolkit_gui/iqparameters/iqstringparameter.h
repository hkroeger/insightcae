#ifndef IQSTRINGPARAMETER_H
#define IQSTRINGPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQStringParameter : public IQParameter
{
public:
  declareType(insight::StringParameter::typeName_());

  IQStringParameter
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

};

#endif // IQSTRINGPARAMETER_H
