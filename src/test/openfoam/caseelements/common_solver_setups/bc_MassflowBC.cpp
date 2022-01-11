#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/boundaryconditions/massflowbc.h"

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
          insert(new MassflowBC(*this, "inlet", boundaryDict, MassflowBC::Parameters()
                                     .set_flowrate(MassflowBC::Parameters::flowrate_volumetric_type(1.0))
                                 ));
        }


    } tc(argv[1]);

    tc.runTest();

  });
}

