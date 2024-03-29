#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>
#include <QIcon>
#include <QColor>

#include "base/parametersetvisualizer.h"
#include "base/supplementedinputdata.h"
#include "cadtypes.h"
#include "iqiscadmodelgenerator.h"

#include "AIS_DisplayMode.hxx"


class IQCADItemModel;
class IQParameterSetModel;
class IQCADModel3DViewer;

namespace insight
{


arma::mat vec3(const QColor& s);


class TOOLKIT_GUI_EXPORT CADParameterSetVisualizer
: public IQISCADModelGenerator,
  public ParameterSetVisualizer
{

  Q_OBJECT

  QThread asyncRebuildThread_;
  std::mutex vis_mtx_;
  IQCADItemModel *model_;
  IQParameterSetModel *psmodel_;

public:

  CADParameterSetVisualizer();
  ~CADParameterSetVisualizer();

  void setModel(IQCADItemModel *model);
  IQCADItemModel *model();
  void setParameterSetModel(IQParameterSetModel* psm);
  IQParameterSetModel* parameterSetModel() const;

  virtual void addDatum(
      const std::string& name,
      insight::cad::DatumPtr dat,
      bool initialVisibility=false);

  virtual void addFeature(
      const std::string& name,
      insight::cad::FeaturePtr feat,
      const insight::cad::FeatureVisualizationStyle& fvs =
        insight::cad::FeatureVisualizationStyle::componentStyle() );

  virtual void addDataset(
      const std::string& name,
      vtkSmartPointer<vtkDataObject> ds );

  void addGeometryToSpatialTransformationParameter(
      const std::string& parameterPath,
      insight::cad::FeaturePtr geom );

  void addVectorBasePoint(
      const std::string& parameterPath,
      const arma::mat& pBase );

  void update(const ParameterSet& ps) override;

  virtual void recreateVisualizationElements();

  struct GUIAction
  {
      QIcon icon;
      QString label, tooltip;
      std::function<void(void)> action;
  };
  typedef std::vector<GUIAction> GUIActionList;

  virtual GUIActionList createGUIActions(
          const std::string& parameterPath,
          QObject* parent,
          IQCADModel3DViewer* viewer);

public Q_SLOTS:
  virtual void visualizeScheduledParameters();

Q_SIGNALS:
  void launchVisualizationCalculation();
  void visualizationCalculationFinished();
  void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
};




class TOOLKIT_GUI_EXPORT MultiCADParameterSetVisualizer
: public CADParameterSetVisualizer
{
  Q_OBJECT

  std::set<CADParameterSetVisualizer*> visualizers_;
  mutable std::set<CADParameterSetVisualizer*> finishedVisualizers_;

public:
  bool hasScheduledParameters() const override;
  bool selectScheduledParameters() override;
  void clearScheduledParameters() override;

  MultiCADParameterSetVisualizer();

  void registerVisualizer(CADParameterSetVisualizer* vis);
  void unregisterVisualizer(CADParameterSetVisualizer* vis);
  int size() const;

  void visualizeScheduledParameters() override;


public Q_SLOTS:
  void onSubVisualizationCalculationFinished();

};


}

#endif // PARAMETERSETVISUALIZER_H
