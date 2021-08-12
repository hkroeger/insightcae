#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/basic/gravity.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_rasmodel.h"


int main(int argc, char*argv[])
{
    return executeTest([=](){

    assertion( argc==3, "Exactly two command line arguments are required!");

    OpenFOAMCaseWithCylinderMesh cm(argv[1]);

    buoyantPimpleFoamNumerics::Parameters p;
    p.deltaT=1;
    p.time_integration.timestep_control = PIMPLESettings::Parameters::timestep_control_fixed_type{};
    p.endTime=1;
    p.boussinesqApproach= (string(argv[2]) == "yes");
    cm.insert(new buoyantPimpleFoamNumerics(cm, p));

    cm.insert(new gravity(cm));

    if (p.boussinesqApproach)
    {
      cm.insert(new boussinesqSinglePhaseTransportProperties(cm));
    }
    else
    {
      cm.insert(new compressibleSinglePhaseThermophysicalProperties(cm));
    }

    cm.insert(new kOmegaSST_RASModel(cm));

    cm.runTest();
    });
}
