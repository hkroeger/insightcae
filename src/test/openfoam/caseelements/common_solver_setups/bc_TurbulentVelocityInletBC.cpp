#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/boundaryconditions/turbulentvelocityinletbc.h"

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
          insert(new TurbulentVelocityInletBC(
                     *this, "inlet", boundaryDict,
                     TurbulentVelocityInletBC::Parameters()
                     .set_turbulence(
                         TurbulentVelocityInletBC::Parameters::turbulence_uniformIntensityAndLengthScale_type
                            {0.05, 0.1}
                         )
                     .set_umean(FieldData::uniformSteady(0,0,1))
                     ));
        }


    } tc(argv[1]);

    tc.runTest();

  });
}
