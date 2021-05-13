#ifndef BLOCKMESH_GUI_H
#define BLOCKMESH_GUI_H

#include "openfoam/blockmesh_templates.h"

#include "cadtypes.h"
#include "parametersetvisualizer.h"

namespace insight
{
namespace bmd
{



class blockMeshDict_Box_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    void recreateVisualizationElements() override;
};




class blockMeshDict_Cylinder_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    void recreateVisualizationElements() override;
    void setIcon(QIcon* i) override;
};




class blockMeshDict_Sphere_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    void recreateVisualizationElements() override;
};



}
}


#endif // BLOCKMESH_GUI_H
