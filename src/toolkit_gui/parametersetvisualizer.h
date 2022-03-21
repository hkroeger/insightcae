#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>

#include "base/parameterset.h"
#include "base/supplementedinputdata.h"
#include "cadtypes.h"
#include "qmodeltree.h"


namespace insight
{


class TOOLKIT_GUI_EXPORT CAD_ParameterSet_Visualizer
: public IQISCADModelContainer,
  public ParameterSet_Visualizer
{

  Q_OBJECT

  QThread asyncRebuildThread_;
  std::mutex vis_mtx_;


public:

  CAD_ParameterSet_Visualizer();
  ~CAD_ParameterSet_Visualizer();

  virtual void addDatum(const std::string& name, insight::cad::DatumPtr dat);
  virtual void addFeature(const std::string& name, insight::cad::FeaturePtr feat, AIS_DisplayMode ds = AIS_Shaded );

  void update(const ParameterSet& ps) override;

  virtual void recreateVisualizationElements();

//  cad::FeaturePtr feature(const std::string& name);
//  void replaceFeature(const std::string& name, insight::cad::FeaturePtr newModel);

public Q_SLOTS:
  void visualizeScheduledParameters();

Q_SIGNALS:
  void launchVisualizationCalculation();
  void visualizationCalculationFinished();
  void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
};




class TOOLKIT_GUI_EXPORT Multi_CAD_ParameterSet_Visualizer
: public CAD_ParameterSet_Visualizer
{
  Q_OBJECT

  std::set<CAD_ParameterSet_Visualizer*> visualizers_;
  mutable std::set<CAD_ParameterSet_Visualizer*> finishedVisualizers_;

public:
  bool hasScheduledParameters() const override;
  bool selectScheduledParameters() override;
  void clearScheduledParameters() override;

  Multi_CAD_ParameterSet_Visualizer();

  void registerVisualizer(CAD_ParameterSet_Visualizer* vis);
  void unregisterVisualizer(CAD_ParameterSet_Visualizer* vis);
  int size() const;

  void recreateVisualizationElements() override;


public Q_SLOTS:
  void onSubVisualizationCalculationFinished();

};


}

#endif // PARAMETERSETVISUALIZER_H
