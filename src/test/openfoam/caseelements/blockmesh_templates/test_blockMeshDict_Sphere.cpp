#include "openfoamblockmesh_template_testcase.h"

#include "openfoam/blockmesh_templates.h"


int main(int, char*argv[])
{
  return executeTest([=](){

    OpenFOAM_blockMeshTemplate_Test tc(argv[1]);

    bmd::blockMeshDict_Sphere::Parameters p;
    p.mesh.n_u = 5;
    tc.insert(new bmd::blockMeshDict_Sphere(tc, p));

    tc.runTest();
  });
}
