#include "openfoamcasewithcylindermesh.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    PimpleFoamCylinderOpenFOAMCase tc(argv[1]);
    try
    {
        tc.runTest();
    }
    catch (insight::ExternalProcessFailed& ex)
    {
        bool ok=false;
        if ((tc.OFversion()==164) && (ex.exeName()=="pimpleFoam"))
        {
            // this will always segfault because of error in leastSquares-related destructor
            if (boost::filesystem::is_directory(tc.dir()/"0.001"))
            {
                ok=true;
            }
        }
        if (!ok) throw ex;
    }
  });
}
