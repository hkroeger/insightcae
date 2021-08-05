#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>

#include "base/parameterset.h"
#include "cadtypes.h"
#include "qmodeltree.h"


namespace insight
{

class TOOLKIT_GUI_EXPORT CAD_ParameterSet_Visualizer
: public IQISCADModelContainer,
  public ParameterSet_Visualizer
{

private:
  Q_OBJECT

  QThread asyncRebuildThread_;
  std::mutex vis_mtx_;

public:

  CAD_ParameterSet_Visualizer();
  ~CAD_ParameterSet_Visualizer();

  void addDatum(const std::string& name, insight::cad::DatumPtr dat);
  void addFeature(const std::string& name, insight::cad::FeaturePtr feat, AIS_DisplayMode ds = AIS_Shaded );

  void update(const ParameterSet& ps) override;

  virtual void recreateVisualizationElements();

//  cad::FeaturePtr feature(const std::string& name);
//  void replaceFeature(const std::string& name, insight::cad::FeaturePtr newModel);

public Q_SLOTS:
  void visualizeScheduledParameters();

Q_SIGNALS:
  void visualizationCalculationFinished();
};


}

#endif // PARAMETERSETVISUALIZER_H
