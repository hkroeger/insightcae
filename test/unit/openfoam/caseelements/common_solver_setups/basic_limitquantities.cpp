#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/limitquantities.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    SimpleFoamCylinderOpenFOAMCase tc(argv[1]);

    limitQuantities::Parameters p;

    p.cells.selection = limitQuantities::Parameters::cells_type::selection_all_type();

    p.limitTemperature = limitQuantities::Parameters::limitTemperature_none_type();

    p.limitVelocity = limitQuantities::Parameters::limitVelocity_limit_type{100.};

    p.limitFields.push_back(
      limitQuantities::Parameters::limitFields_default_type
        {"p", limitQuantities::Parameters::limitFields_default_type::scalar, -100, 100}
    );

    tc.insert(new limitQuantities(tc, p));

    tc.runTest();

  });
}
