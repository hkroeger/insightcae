#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
#include "openfoam/caseelements/basic/providefields.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public SimpleFoamOpenFOAMCase
    {
    public:
        Case(const std::string& ofe) : SimpleFoamOpenFOAMCase(ofe) {}

        void createWallBC(OFDictData::dict& bd) override
        {
          insert(new WallBC(*this, "walls", bd, WallBC::Parameters()
                            .set_heattransfer(
                                std::make_shared<HeatBC::ExternalWallBC>(
                                    HeatBC::ExternalWallBC::Parameters()
                                    .set_kappaSource(HeatBC::ExternalWallBC::Parameters::kappaSource_lookup_type{"kappa"})
                                    .set_heatflux(HeatBC::ExternalWallBC::Parameters::heatflux_fixedHeatTransferCoeff_type
                                                  { 10. /*heatTransferCoeffcient*/,
                                                    300. /*ambientTemperature*/ } )
                                    )
                                ))
                            );

          insert(new provideFields(
                      *this, provideFields::Parameters()
                      .set_createScalarFields(
                        { provideFields::Parameters::createScalarFields_default_type{
                            "kappa",
                            {0,0,0,0},
                            1.0 /*kappa*/ }
                        } )
                      ));
        }

    } tc(argv[1]);

    PassiveScalar::Parameters p;
    p.set_fieldname("T");
    p.set_underrelax(0.7);
    tc.insert(new PassiveScalar(tc, p));

    tc.runTest();
  });
}
