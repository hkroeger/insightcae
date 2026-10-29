#ifndef INSIGHT_BLOCKMESHTEMPLATEVISUALIZER_H
#define INSIGHT_BLOCKMESHTEMPLATEVISUALIZER_H

#include "toolkit_gui_export.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/blockmesh_templates.h"

#include "cadparametersetvisualizer.h"
#include "blockmeshvisualization.h"

namespace insight
{
namespace bmd
{




/**
 * @brief The BlockMeshTemplate_ParameterSet_Visualizer class
 * visualizes any blockMesh template by creating its blocking
 * in a dummy case and displaying the resulting blocks.
 */
template<class BlockMeshTemplateType>
class BlockMeshTemplate_ParameterSet_Visualizer
 : public CADParameterSetModelVisualizer
{
    std::string blockMeshName_ = BlockMeshTemplateType::typeName;

public:
    typedef typename BlockMeshTemplateType::Parameters Parameters;

    using CADParameterSetModelVisualizer::CADParameterSetModelVisualizer;

    void setBlockMeshName(const std::string& blockMeshName)
    {
        blockMeshName_ = blockMeshName;
    }

    std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() override
    {
        return nullptr;
    }

    void recreateVisualizationElements() override
    {
        CurrentExceptionContext ex("computing visualization of blockMesh template "+blockMeshName_);

        OpenFOAMCase dummy;
        BlockMeshTemplateType bm(dummy, parameters());
        bm.create_bmd();

        addBlockingToVisualizer(*this, bm, blockMeshName_);
    }
};




}
}

#endif // INSIGHT_BLOCKMESHTEMPLATEVISUALIZER_H
