#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public SimpleFoamCylinderOpenFOAMCase
    {
    public:
        Case(const std::string& ofe) : SimpleFoamCylinderOpenFOAMCase(ofe) {}

        void createWallBC(OFDictData::dict& bd) override
        {
          insert(new WallBC(*this, "walls", bd, WallBC::Parameters()
                            .set_heattransfer(
                                std::make_shared<HeatBC::FixedTemperatureBC>(
                                    HeatBC::FixedTemperatureBC::Parameters()
                                    .set_T(FieldData::uniformSteady(400.))
                                    )
                                ))
                            );
        }

    } tc(argv[1]);

    PassiveScalar::Parameters p;
    p.set_fieldname("T");
    p.set_underrelax(0.7);
    tc.insert(new PassiveScalar(tc, p));

    tc.runTest();
  });
}
