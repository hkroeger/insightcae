#ifndef IQCADGEOMETRYPARAMETER_H
#define IQCADGEOMETRYPARAMETER_H




#include "iqparameter.h"
#include "cadgeometryparameter.h"




class IQCADGeometryParameter : public IQParameter
{
public:
  declareType(insight::CADGeometryParameter::typeName_());

  IQCADGeometryParameter
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




#endif // IQCADGEOMETRYPARAMETER_H
