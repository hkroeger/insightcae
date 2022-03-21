#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/laplacianfoamnumerics.h"

int main(int argc, char*argv[])
{
    return executeTest([=](){

        assertion( argc==2, "Exactly one command line arguments are required!");

        OpenFOAMCaseWithCylinderMesh cm(argv[1]);

        laplacianFoamNumerics::Parameters p;
        p.deltaT=1;
        p.endTime=1;
        cm.insert(new laplacianFoamNumerics(cm, p));

        cm.runTest();
    });
}
