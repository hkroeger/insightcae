#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include <QObject>
#include <QThread>

#include "base/parameterset.h"
#include "cadtypes.h"
#include "qmodeltree.h"


namespace insight
{

class CAD_ParameterSet_Visualizer
: public IQISCADModel,
  public ParameterSet_Visualizer
{
public:

  struct UsageTracker
  {
    QModelTree* mt_;
    std::set<std::string> removedDatums_, removedFeatures_;

    UsageTracker(QModelTree* mt);
    void cleanupModelTree();
  };

  enum DisplayStyle { Shaded, Wireframe };

private:
  Q_OBJECT

  QThread asyncRebuildThread_;
  UsageTracker* ut_;

public:

  CAD_ParameterSet_Visualizer();
  ~CAD_ParameterSet_Visualizer();

  void addDatum(const std::string& name, insight::cad::DatumPtr dat);
  void addFeature(const std::string& name, insight::cad::FeaturePtr feat, DisplayStyle ds = DisplayStyle::Shaded );

  void update(const ParameterSet& ps) override;

  virtual void recreateVisualizationElements(UsageTracker* ut);

//  cad::FeaturePtr feature(const std::string& name);
//  void replaceFeature(const std::string& name, insight::cad::FeaturePtr newModel);

public Q_SLOTS:
  void visualizeScheduledParameters();

Q_SIGNALS:
  void GUINeedsUpdate();
  void visualizationCalculationFinished();
};


}

#endif // PARAMETERSETVISUALIZER_H
