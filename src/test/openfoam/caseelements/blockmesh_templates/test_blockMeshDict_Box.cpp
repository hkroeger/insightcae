#include "openfoamblockmesh_template_testcase.h"

#include "openfoam/blockmesh_templates.h"


int main(int, char*argv[])
{
  return executeTest([=](){

    OpenFOAM_blockMeshTemplate_Test tc(argv[1]);

    bmd::blockMeshDict_Box::Parameters p;
    p.mesh.resolution = bmd::blockMeshDict_Box::Parameters::mesh_type::resolution_cubical_type{2};
    tc.insert(new bmd::blockMeshDict_Box(tc, p));

    tc.runTest();
  });
}
