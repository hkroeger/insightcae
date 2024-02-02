#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/laplacianfoamnumerics.h"

int main(int argc, char*argv[])
{
    std::unique_ptr<OpenFOAMCaseWithCylinderMesh> cm;

    return executeTest([&](){

        assertion( argc==2, "Exactly one command line arguments are required!");

        cm.reset(new OpenFOAMCaseWithCylinderMesh(argv[1]));

        laplacianFoamNumerics::Parameters p;
        p.deltaT=1;
        p.writeControl=laplacianFoamNumerics::Parameters::timeStep;
        p.writeInterval=1;
        p.endTime=1;
        cm->insert(new laplacianFoamNumerics(*cm, p));

        try
        {
            cm->runTest();
        }
        catch (insight::ExternalProcessFailed& ex)
        {
            bool ok=false;
            if ((cm->OFversion()==164) && (ex.exeName()=="laplacianFoam"))
            {
                // this will always segfault because of error in leastSquares-related destructor
                if (boost::filesystem::is_directory(cm->dir()/"1"))
                {
                    ok=true;
                }
            }
            if (!ok) throw ex;
        }
    });
}
