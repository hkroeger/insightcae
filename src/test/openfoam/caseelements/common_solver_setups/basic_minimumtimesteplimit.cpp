#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/minimumtimesteplimit.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    PimpleFoamOpenFOAMCase tc(argv[1]);

    minimumTimestepLimit::Parameters p;
    tc.insert(new minimumTimestepLimit(tc, p));

    tc.runTest();

  });
}
