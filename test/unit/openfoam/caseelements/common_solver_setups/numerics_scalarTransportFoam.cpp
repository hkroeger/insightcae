#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/scalartransportfoamnumerics.h"

int main(int argc, char*argv[])
{
    return executeTest([=](){

        assertion( argc==2, "Exactly one command line arguments are required!");

        OpenFOAMCaseWithCylinderMesh cm(argv[1]);

        scalarTransportFoamNumerics::Parameters p;
        p.deltaT=1;
        p.endTime=1;
        cm.insert(new scalarTransportFoamNumerics(cm, p));

        cm.runTest();
    });
}
