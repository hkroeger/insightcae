#ifndef IQMATRIXPARAMETER_H
#define IQMATRIXPARAMETER_H

#include "toolkit_gui_export.h"


#include <iqparameter.h>

#include "base/parameters/matrixparameter.h"

class TOOLKIT_GUI_EXPORT IQMatrixParameter
    : public IQSpecializedParameter<insight::MatrixParameter>
{
public:
  declareType(insight::MatrixParameter::typeName_());

  IQMatrixParameter
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  QString valueText() const override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

};

#endif // IQMATRIXPARAMETER_H
