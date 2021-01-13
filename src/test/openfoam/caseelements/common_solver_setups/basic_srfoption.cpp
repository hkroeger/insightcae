#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/srfoption.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    PimpleFoamOpenFOAMCase tc(argv[1]);

    SRFoption::Parameters p;
    p.set_rpm(100);
    p.set_axis(vec3(0,0,1));
    p.set_origin(vec3(0,0,0));
    tc.insert(new SRFoption(tc, p));

    tc.runTest();
  });
}
