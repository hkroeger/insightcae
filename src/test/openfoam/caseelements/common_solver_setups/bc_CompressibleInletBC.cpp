#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/boundaryconditions/compressibleinletbc.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public SteadyCompressibleOpenFOAMCase
    {
    public:
        Case(const std::string& ofe) : SteadyCompressibleOpenFOAMCase(ofe) {};

        void createInletBC(OFDictData::dict& bd) override
        {
            CompressibleInletBC::Parameters p;
            p.velocity.fielddata=VelocityInletBC::Parameters::velocity_type::fielddata_uniformSteady_type{ 1.0*flowDir_ };
            p.pressure=1e5;
            insert(new CompressibleInletBC(*this, "inlet", bd, p));
        }

    } tc(argv[1]);

    tc.runTest();

  });
}
