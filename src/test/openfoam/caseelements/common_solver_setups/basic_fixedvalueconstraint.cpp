#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/fixedvalueconstraint.h"

#include "openfoam/openfoamtools.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public CyclicPimpleFoamOpenFOAMCase
    {
    public:
      Case(const std::string& ofe) : CyclicPimpleFoamOpenFOAMCase(ofe) {}

      void createMesh() override
      {
        CyclicPimpleFoamOpenFOAMCase::createMesh();
        setSet(*this, dir_,
               {
                 "cellSet drive new cylinderToCell (0 0 0.25) (0 0 0.75) 0.25"
               }
               );
        setsToZones(*this, dir_);
      }
    } tc(argv[1]);

    fixedValueConstraint::Parameters p;
    p.set_name("driver");
    p.set_fieldName("U");
    p.set_zoneName("drive");
    p.set_value(fixedValueConstraint::Parameters::value_vector_type{vec3(0,0,1)});
    tc.insert(new fixedValueConstraint(tc, p));

    tc.runTest();

  });
}
