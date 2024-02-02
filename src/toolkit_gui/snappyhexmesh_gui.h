#ifndef SNAPPYHEXMESH_GUI_H
#define SNAPPYHEXMESH_GUI_H

#include "toolkit_gui_export.h"


#include "cadparametersetvisualizer.h"
#include "openfoam/snappyhexmesh.h"
#include "cadtypes.h"

namespace insight
{


class TOOLKIT_GUI_EXPORT snappyHexMeshConfiguration_ParameterSet_Visualizer
 : public CADParameterSetVisualizer
{
public:
    virtual void recreateVisualizationElements();
    virtual void setIcon(QIcon* icon);
};


}


#endif // SNAPPYHEXMESH_GUI_H
