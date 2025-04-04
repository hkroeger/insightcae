#ifndef IQBOOLPARAMETER_H
#define IQBOOLPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/simpleparameter.h"

class TOOLKIT_GUI_EXPORT IQBoolParameter
    : public IQSpecializedParameter<insight::BoolParameter>
{
public:
  declareType(insight::BoolParameter::typeName_());

  IQBoolParameter
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;
  bool setValue(QVariant value) override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer ) override;

};

#endif // IQBOOLPARAMETER_H
