#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/boundaryconditions/suctioninletbc.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public PimpleFoamOpenFOAMCase
    {
    public:
        Case(const std::string& ofe) : PimpleFoamOpenFOAMCase(ofe) {}

        void createInletBC(OFDictData::dict &boundaryDict) override
        {
          insert(new SuctionInletBC(*this, "inlet", boundaryDict, SuctionInletBC::Parameters()
                                     .set_pressure(1.01e5)
                                     .set_rho(1.2)
                                 ));
        }


    } tc(argv[1]);

    tc.runTest();

  });
}
