#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>

#include "base/parametersetvisualizer.h"
#include "base/supplementedinputdata.h"
#include "cadtypes.h"
#include "iqiscadmodelgenerator.h"

#include "AIS_DisplayMode.hxx"


class IQCADItemModel;
class IQParameterSetModel;

namespace insight
{


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

  virtual void addDatum(const std::string& name, insight::cad::DatumPtr dat);
  virtual void addFeature(const std::string& name, insight::cad::FeaturePtr feat, AIS_DisplayMode ds = AIS_Shaded );
  virtual void addDataset(const std::string& name, vtkSmartPointer<vtkDataObject> ds);

  void addGeometryToSpatialTransformationParameter(
              const std::string& parameterPath,
              insight::cad::FeaturePtr geom );

  void addVectorBasePoint(
          const std::string& parameterPath, const arma::mat& pBase );

  void update(const ParameterSet& ps) override;

  virtual void recreateVisualizationElements();

public Q_SLOTS:
  void visualizeScheduledParameters();

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

  void recreateVisualizationElements() override;


public Q_SLOTS:
  void onSubVisualizationCalculationFinished();

};


}

#endif // PARAMETERSETVISUALIZER_H
