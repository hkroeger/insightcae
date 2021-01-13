
#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/constantpressuregradientsource.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    CyclicPimpleFoamOpenFOAMCase tc(argv[1]);


    tc.insert(
          new ConstantPressureGradientSource(
                tc, ConstantPressureGradientSource::Parameters()
                .set_gradp(vec3( 1., 0., 0. ))
            )
          );

    tc.runTest();

  });
}
