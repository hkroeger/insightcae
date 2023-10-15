#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/dynamicmesh/solidbodymotiondynamicmesh.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    PimpleFoamCylinderOpenFOAMCase tc(argv[1]);

    tc.insert(new solidBodyMotionDynamicMesh(
               tc,
               solidBodyMotionDynamicMesh::Parameters()
               .set_motion(
                   solidBodyMotionDynamicMesh::Parameters::motion_rotation_type{
                       vec3(0,0,0),
                       vec3(0,0,1),
                       1.0
                   }
                   )
               .set_zonename("wholeDomain")
               ));

    tc.runTest();
  });
}
