#ifndef IQISCADMODELGENERATOR_H
#define IQISCADMODELGENERATOR_H

#include "toolkit_gui_export.h"

#include <QObject>


#include "cadtypes.h"

#include <vtkSmartPointer.h>
#include <vtkDataObject.h>



class TOOLKIT_GUI_EXPORT IQISCADModelGenerator
    : public QObject
{
  Q_OBJECT

public:
  IQISCADModelGenerator(QObject* parent=nullptr);

Q_SIGNALS:

  void createdVariable(
        const QString& sn,
        insight::cad::ScalarPtr sv );

  void createdVariable(
      const QString& sn,
      insight::cad::VectorPtr vv,
      insight::cad::VectorVariableType vt );

  void createdFeature(
      const QString& sn,
      insight::cad::FeaturePtr sm,
      bool is_component,
      const insight::cad::FeatureVisualizationStyle& fvs = insight::cad::FeatureVisualizationStyle() );

  void createdDatum(
      const QString& sn,
      insight::cad::DatumPtr dm,
      bool initialVisibility=false );

  void createdEvaluation(
      const QString& sn,
      insight::cad::PostprocActionPtr em,
      bool visible );

  void createdDataset(
      const QString& sn,
      vtkSmartPointer<vtkDataObject> ds,
      bool visible );

  void statusMessage(const QString& msg, double timeout=0);
  void statusProgress(int step, int totalSteps);

};


#endif // IQISCADMODELGENERATOR_H
