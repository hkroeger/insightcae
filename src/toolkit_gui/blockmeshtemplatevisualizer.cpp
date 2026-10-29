#include "blockmeshtemplatevisualizer.h"

#include "blockmesh_cylwedge.h"
#include "blockmesh_cylwedgeortho.h"
#include "blockmesh_curvedcylinder.h"
#include "blockmesh_tubemesh.h"


namespace insight
{
namespace bmd
{


#define addBlockMeshTemplateVisualizer(BMT) \
addToStaticFunctionTable2( \
    CADParameterSetModelVisualizer, \
    VisualizerFunctions, visualizerForOpenFOAMCaseElement, \
    BMT, &newVisualizer<BlockMeshTemplate_ParameterSet_Visualizer<BMT> >)


addBlockMeshTemplateVisualizer(blockMeshDict_Box);
addBlockMeshTemplateVisualizer(blockMeshDict_Cylinder);
addBlockMeshTemplateVisualizer(blockMeshDict_Sphere);
addBlockMeshTemplateVisualizer(blockMeshDict_CylWedge);
addBlockMeshTemplateVisualizer(blockMeshDict_CylWedgeOrtho);
addBlockMeshTemplateVisualizer(blockMeshDict_CurvedCylinder);
addBlockMeshTemplateVisualizer(blockMeshDict_TubeMesh);




addToStaticFunctionTable2(
    CADParameterSetModelVisualizer,
    IconFunctions, iconForOpenFOAMCaseElement,
    blockMeshDict_Cylinder, [](){ return QIcon(":symbole/bmd_cyl.svg"); });



}
}
