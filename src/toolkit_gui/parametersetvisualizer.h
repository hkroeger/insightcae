#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "base/parameterset.h"
#include "cadtypes.h"

namespace insight
{

class CAD_ParameterSet_Visualizer
: public ParameterSet_Visualizer
{
    QModelTree* mt_;
    std::set<std::string> removedDatums_, removedFeatures_;
public:

    void addDatum(const std::string& name, insight::cad::DatumPtr dat);
    void addFeature(const std::string& name, insight::cad::FeaturePtr feat);

    virtual void updateVisualizationElements(QoccViewWidget*, QModelTree*);

    virtual void recreateVisualizationElements(QoccViewWidget*);
};


}

#endif // PARAMETERSETVISUALIZER_H
