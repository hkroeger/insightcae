#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/boundaryconditions/mappedvelocityinletbc.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public PimpleFoamCylinderOpenFOAMCase
    {
    public:
        Case(const std::string& ofe) : PimpleFoamCylinderOpenFOAMCase(ofe) {}

        void createInletBC(OFDictData::dict &boundaryDict) override
        {
          insert(new MappedVelocityInletBC(
                     *this, "inlet", boundaryDict,
                     MappedVelocityInletBC::Parameters()
                     .set_distance(
                         meshParameters_.geometry.ex*meshParameters_.geometry.L*0.1 )
                     ));
        }


    } tc(argv[1]);

    tc.runTest();

  });
}
