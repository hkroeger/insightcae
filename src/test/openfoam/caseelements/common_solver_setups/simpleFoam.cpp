#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/turbulencemodelcaseelements.h"


int main(int, char*argv[])
{
    return executeTest([=](){
    OpenFOAMCaseWithCylinderMesh tc(argv[1]);

    steadyIncompressibleNumerics::Parameters p;
    p.pinternal=1e5;
    p.endTime=1;
    tc.insert(new steadyIncompressibleNumerics(tc, p));

    tc.insert(new singlePhaseTransportProperties(tc));

    tc.insert(new kOmegaSST_RASModel(tc));

    tc.runTest();
    });
}
