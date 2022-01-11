#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/boundaryconditions/cyclicggibc.h"

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
          insert(new CyclicGGIBC(*this, "inlet", boundaryDict, CyclicGGIBC::Parameters()
                                 .set_separationOffset(meshParameters_.geometry.L*meshParameters_.geometry.ex)
                                 .set_shadowPatch("outlet")
                                 ));
          insert(new CyclicGGIBC(*this, "outlet", boundaryDict, CyclicGGIBC::Parameters()
                                 .set_separationOffset(-meshParameters_.geometry.L*meshParameters_.geometry.ex)
                                 .set_shadowPatch("inlet")
                                 ));
        }

        void createOutletBC(OFDictData::dict &) override
        {
          // skip
        }

    } tc(argv[1]);

    tc.runTest();

  });
}
