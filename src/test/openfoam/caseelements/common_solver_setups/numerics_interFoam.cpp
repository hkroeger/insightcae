#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/interfoamnumerics.h"
#include "openfoam/caseelements/basic/gravity.h"
#include "openfoam/caseelements/basic/twophasetransportproperties.h"

int main(int argc, char* argv[])
{
    return executeTest([=](){

        assertion( argc==2, "Exactly one command line arguments are required!");

        OpenFOAMCaseWithCylinderMesh cm(argv[1]);

        interFoamNumerics::Parameters p;
        p.deltaT=1;
        MultiphasePIMPLESettings::Parameters ti;
        MultiphasePIMPLESettings::Parameters::pressure_velocity_coupling_PIMPLE_type pimple;
        pimple.max_nOuterCorrectors=1;
        pimple.nCorrectors=1;
        ti.pressure_velocity_coupling=pimple;
        ti.timestep_control = MultiphasePIMPLESettings::Parameters::timestep_control_fixed_type{};
        p.time_integration=ti;
        p.endTime=1;
        cm.insert(new interFoamNumerics(cm, p));

        cm.insert(new gravity(cm));
        cm.insert(new twoPhaseTransportProperties(cm, twoPhaseTransportProperties::Parameters()
          .set_nu1(1e-6)
          .set_rho1(1025.)
        ));
        cm.insert(new kOmegaSST_RASModel(cm));

        cm.runTest();
    });
}
