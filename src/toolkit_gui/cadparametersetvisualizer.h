#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>
#include <QIcon>
#include <QColor>
#include <QTimer>

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

std::string visPath(const std::vector<std::string>& path);

std::string visPath();

template<typename T, typename... Args>
std::string visPath(T t, Args... args) // recursive variadic function
{
    std::string head = t;
    std::string tail = visPath(args...);
    return head + ((head.size()&&tail.size())?"/":"") + tail;
}

class TOOLKIT_GUI_EXPORT CADParameterSetVisualizer
: public IQISCADModelGenerator,
  public ParameterSetVisualizer
{

  Q_OBJECT

protected:
  std::unique_ptr<boost::thread> asyncRebuildThread_;
  IQCADItemModel *model_;
  IQParameterSetModel *psmodel_;

  QTimer timerToUpdate_;

public:

  CADParameterSetVisualizer();
  ~CADParameterSetVisualizer();

  void stopVisualizationComputation();

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
  void visualizationCalculationFinished(bool success);
  void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
  void visualizationComputationError(insight::Exception ex);
};




class TOOLKIT_GUI_EXPORT MultiCADParameterSetVisualizer
: public CADParameterSetVisualizer
{
  Q_OBJECT

  std::set<CADParameterSetVisualizer*> visualizers_;
  mutable std::map<CADParameterSetVisualizer*, bool> finishedVisualizers_;

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
  void onSubVisualizationCalculationFinished(bool success);

};


}

#endif // PARAMETERSETVISUALIZER_H
