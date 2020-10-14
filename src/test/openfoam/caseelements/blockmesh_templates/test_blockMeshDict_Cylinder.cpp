#include "openfoamblockmesh_template_testcase.h"

#include "openfoam/blockmesh_templates.h"


int main(int, char*argv[])
{
    OpenFOAM_blockMeshTemplate_Test tc(argv[1]);

    bmd::blockMeshDict_Cylinder::Parameters p;
    p.mesh.resolution = bmd::blockMeshDict_Cylinder::Parameters::mesh_type::resolution_cubical_type{2};
    tc.insert(new bmd::blockMeshDict_Cylinder(tc, p));

    tc.runTest();
    return 0;
}
