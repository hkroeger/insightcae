#include "openfoamcasewithcylindermesh.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    PimpleFoamOpenFOAMCase tc(argv[1]);
    tc.runTest();
  });
}
