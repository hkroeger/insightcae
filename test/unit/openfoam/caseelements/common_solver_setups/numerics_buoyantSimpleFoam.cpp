#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/basic/gravity.h"
#include "openfoam/caseelements/thermodynamics/compressiblesinglephasethermophysicalproperties.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h"


int main(int argc, char*argv[])
{
    return executeTest([=](){

        assertion( argc==3, "Exactly two command line arguments are required!");

        OpenFOAMCaseWithCylinderMesh cm(argv[1]);

        buoyantSimpleFoamNumerics::Parameters p;
        p.deltaT=1;
        p.endTime=1;
        cm.insert(new buoyantSimpleFoamNumerics(cm, p));

        cm.insert(new gravity(cm));

        cm.insert(new compressibleSinglePhaseThermophysicalProperties(cm));

        cm.insert(new kOmegaSST_RASModel(cm));

        cm.runTest();
    });
}
