#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "base/parameterset.h"
#include "cadtypes.h"

#include <QObject>

namespace insight
{

class CAD_ParameterSet_Visualizer
: public QObject,
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

private:
  Q_OBJECT

  UsageTracker* ut_;

public:

  void addDatum(const std::string& name, insight::cad::DatumPtr dat);
  void addFeature(const std::string& name, insight::cad::FeaturePtr feat);

  void update(const ParameterSet& ps) override;

  virtual void recreateVisualizationElements(UsageTracker* ut);

Q_SIGNALS:
  void GUINeedsUpdate();
};


}

#endif // PARAMETERSETVISUALIZER_H
