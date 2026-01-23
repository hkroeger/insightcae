#ifndef IQCADGEOMETRYPARAMETER_H
#define IQCADGEOMETRYPARAMETER_H




#include "iqparameter.h"
#include "cadgeometryparameter.h"




class IQCADGeometryParameter
    : public IQSpecializedParameter<insight::CADGeometryParameter>
{
public:
  declareType(insight::CADGeometryParameter::typeName_());

  IQCADGeometryParameter
  (
      QObject* parent,
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* element
  );


  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer) override;
};




#endif // IQCADGEOMETRYPARAMETER_H
