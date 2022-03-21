#include "openfoamblockmesh_template_testcase.h"

#include "base/tools.h"
#include "base/casedirectory.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"

OpenFOAM_blockMeshTemplate_Test::OpenFOAM_blockMeshTemplate_Test(const string &OFEname)
    : OpenFOAMTestCase(OFEname)
{
  insert(new MeshingNumerics(*this));
}

void OpenFOAM_blockMeshTemplate_Test::runTest()
{
  CaseDirectory dir(false);
  createOnDisk(dir);
  executeCommand(dir, "blockMesh");
}
