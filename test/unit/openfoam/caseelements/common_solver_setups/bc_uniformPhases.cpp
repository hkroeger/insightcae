#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public SimpleFoamCylinderOpenFOAMCase
    {
    public:
        Case(const std::string& ofe) : SimpleFoamCylinderOpenFOAMCase(ofe) {}

        void createInletBC(OFDictData::dict& bd) override
        {
          insert(new VelocityInletBC(*this, "walls", bd, VelocityInletBC::Parameters()
                            .set_velocity(FieldData::uniformSteady( 1.0*flowDir_ ))
                            .set_phasefractions(
                                std::make_shared<multiphaseBC::uniformPhases>(
                                    multiphaseBC::uniformPhases::mixture({
                                                            {"Y", 0.4}
                                                        })
                                    )
                                ))
                            );
        }

    } tc(argv[1]);

    PassiveScalar::Parameters p;
    p.set_fieldname("Y");
    p.set_underrelax(0.7);
    tc.insert(new PassiveScalar(tc, p));

    tc.runTest();
  });
}
