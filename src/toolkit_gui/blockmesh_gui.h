#ifndef BLOCKMESH_GUI_H
#define BLOCKMESH_GUI_H

#include "toolkit_gui_export.h"


#include "openfoam/blockmesh_templates.h"

#include "cadtypes.h"
#include "parametersetvisualizer.h"

namespace insight
{
namespace bmd
{



class TOOLKIT_GUI_EXPORT blockMeshDict_Box_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    void recreateVisualizationElements() override;
};




class TOOLKIT_GUI_EXPORT blockMeshDict_Cylinder_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    void recreateVisualizationElements() override;
    void setIcon(QIcon* i) override;
};




class TOOLKIT_GUI_EXPORT blockMeshDict_Sphere_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    void recreateVisualizationElements() override;
};



}
}


#endif // BLOCKMESH_GUI_H
