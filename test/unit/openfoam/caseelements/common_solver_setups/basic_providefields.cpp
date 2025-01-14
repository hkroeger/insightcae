#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/providefields.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    SimpleFoamCylinderOpenFOAMCase tc(argv[1]);

    provideFields::Parameters p;
    p.set_createScalarFields
        ( {
            provideFields::Parameters::createScalarFields_default_type
                              {"T", {0,0,0,1}, 300.0}
          } );
    tc.insert(new provideFields(tc, p));

    tc.runTest();

  });
}
