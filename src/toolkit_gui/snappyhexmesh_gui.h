#ifndef SNAPPYHEXMESH_GUI_H
#define SNAPPYHEXMESH_GUI_H

#include "toolkit_gui_export.h"


#include "cadparametersetvisualizer.h"
#include "openfoam/snappyhexmesh.h"
#include "cadtypes.h"

namespace insight
{


class TOOLKIT_GUI_EXPORT snappyHexMeshConfiguration_ParameterSet_Visualizer
 : public CADParameterSetModelVisualizer
{
public:
    using CADParameterSetModelVisualizer::CADParameterSetModelVisualizer;

    std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() override;
    void recreateVisualizationElements() override;
};


}


#endif // SNAPPYHEXMESH_GUI_H
