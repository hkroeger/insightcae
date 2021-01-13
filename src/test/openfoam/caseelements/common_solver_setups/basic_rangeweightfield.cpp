#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/rangeweightfield.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    SimpleFoamOpenFOAMCase tc(argv[1]);

    rangeWeightField::Parameters p;
    p.set_sourceFieldName("p");
    p.set_outputFieldName("prange");
    tc.insert(new rangeWeightField(tc, p));

    tc.runTest();
  });
}
