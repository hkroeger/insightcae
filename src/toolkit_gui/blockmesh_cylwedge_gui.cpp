#include "blockmesh_cylwedge.h"

#include "base/tools.h"
#include "base/units.h"

#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"

#include "cadfeatures.h"
#include "datum.h"

namespace insight
{
namespace bmd
{




ParameterSet_VisualizerPtr blockMeshDict_CylWedge_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_CylWedge_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_CylWedge, visualizer, blockMeshDict_CylWedge_visualizer);


void blockMeshDict_CylWedge_ParameterSet_Visualizer::recreateVisualizationElements(UsageTracker *ut, const std::string& featureName)
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements(ut);

  Parameters p(ps_);

  OpenFOAMCase oc(OFEs::getCurrentOrPreferred());
  blockMeshDict_CylWedge bcw( oc, ps_ );

  auto dom =
      cad::Revolution::create(
        cad::Quad::create(
          cad::matconst(p.geometry.p0 + bcw.er_*p.geometry.d*0.5),
          cad::matconst(bcw.er_*(p.geometry.D - p.geometry.d)*0.5),
          cad::matconst(bcw.ex_*p.geometry.L)
          ),
        cad::matconst(p.geometry.p0),
        cad::matconst(p.geometry.ex),
        cad::scalarconst(p.geometry.wedge_angle*SI::deg),
        true
        )
      ;

  addFeature( featureName,
              cad::Compound::create(cad::CompoundFeatureList({dom})),
              Wireframe
              );
}



void blockMeshDict_CylWedge_ParameterSet_Visualizer::recreateVisualizationElements(UsageTracker *ut)
{
  recreateVisualizationElements(ut, "blockMeshDict_CylWedge");
}

}
}
