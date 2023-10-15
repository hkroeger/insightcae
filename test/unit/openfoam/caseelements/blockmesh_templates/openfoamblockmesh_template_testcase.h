#ifndef OPENFOAM_BLOCKMESH_TEMPLATES_TEST_H
#define OPENFOAM_BLOCKMESH_TEMPLATES_TEST_H

#include "../openfoamtestcase.h"

class OpenFOAM_blockMeshTemplate_Test
        : public OpenFOAMTestCase
{
public:
  OpenFOAM_blockMeshTemplate_Test(const string& OFEname);

  void runTest() override;
};

#endif // OPENFOAM_BLOCKMESH_TEMPLATES_TEST_H
