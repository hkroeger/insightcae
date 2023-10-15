#include "openfoamcasewithcylindermesh.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

      insight::assertion(argc==2, "expected exactly one command line argument");

      SteadyCompressibleOpenFOAMCase tc(argv[1]);

//      try
//      {
          tc.runTest();
//      }
//      catch (insight::ExternalProcessFailed& ex)
//      {
//          bool ok=false;
//          if ((tc.OFversion()==164) && (ex.exeName()=="rhoSimpleFoam"))
//          {
//              // this will always segfault because of error in leastSquares-related destructor
//              if (boost::filesystem::is_directory(tc.dir()/"1"))
//              {
//                  ok=true;
//              }
//          }
//          if (!ok) throw ex;
//      }
  });
}
