#include "openfoamcasewithcylindermesh.h"


int main(int, char*argv[])
{
    std::unique_ptr<SimpleFoamCylinderOpenFOAMCase> tc;
    return executeTest([&](){
        tc.reset(new SimpleFoamCylinderOpenFOAMCase(argv[1]));
        try
        {
            tc->runTest();
        }
        catch (insight::ExternalProcessFailed& ex)
        {
            bool ok=false;
            if ((tc->OFversion()==164) && (ex.exeName()=="simpleFoam"))
            {
                // this will always segfault because of error in leastSquares-related destructor
                if (boost::filesystem::is_directory(tc->dir()/"1"))
                {
                    ok=true;
                }
            }
            if (!ok) throw ex;
        }
    });
}
