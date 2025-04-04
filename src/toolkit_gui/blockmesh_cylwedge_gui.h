#ifndef BLOCKMESH_CYLWEDGE_GUI_H
#define BLOCKMESH_CYLWEDGE_GUI_H

#include "blockmesh_cylwedge.h"
#include "cadparametersetvisualizer.h"


namespace insight
{
namespace bmd
{




class TOOLKIT_GUI_EXPORT blockMeshDict_CylWedge_ParameterSet_Visualizer
 : public CADParameterSetModelVisualizer
{
public:
    typedef blockMeshDict_CylWedge::Parameters Parameters;
    std::string blockMeshName_ = "blockMeshDict_CylWedge";

public:
    using CADParameterSetModelVisualizer::CADParameterSetModelVisualizer;

    std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() override;
    void setBlockMeshName(const std::string& blockMeshName);
    void recreateVisualizationElements() override;
};




}
}

#endif // BLOCKMESH_CYLWEDGE_GUI_H
