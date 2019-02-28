#ifndef SNAPPYHEXMESH_GUI_H
#define SNAPPYHEXMESH_GUI_H

#include "openfoam/snappyhexmesh.h"
#include "cadtypes.h"

namespace insight
{


class snappyHexMeshConfiguration_ParameterSet_Visualizer
 : public ParameterSet_Visualizer
{
public:
    typedef snappyHexMeshConfiguration::Parameters Parameters;
    typedef std::map<std::string, cad::FeaturePtr> ItemList;

protected:
    ItemList items_;

public:
    virtual void update(const ParameterSet& ps);
    virtual void updateVisualizationElements(QoccViewWidget*, QModelTree*);
};


}


#endif // SNAPPYHEXMESH_GUI_H
