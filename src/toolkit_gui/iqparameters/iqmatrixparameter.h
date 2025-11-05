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
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* element
  );

  QVariant value() const override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;

};

#endif // IQMATRIXPARAMETER_H
