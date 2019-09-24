#ifndef SNAPPYHEXMESH_GUI_H
#define SNAPPYHEXMESH_GUI_H

#include "parametersetvisualizer.h"
#include "openfoam/snappyhexmesh.h"
#include "cadtypes.h"

namespace insight
{


class snappyHexMeshConfiguration_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    virtual void recreateVisualizationElements(UsageTracker *ut);
    virtual void setIcon(QIcon* icon);
};


}


#endif // SNAPPYHEXMESH_GUI_H
