#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/porouszone.h"
#include "openfoam/openfoamtools.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case
        : public SimpleFoamCylinderOpenFOAMCase
    {
    public:
      Case(const std::string& ofe)
        : SimpleFoamCylinderOpenFOAMCase(ofe)
      {}

       void createMesh() override
       {
         SimpleFoamCylinderOpenFOAMCase::createMesh();

         setSet(*this, dir_, {
                  "cellSet allCells new boxToCell (-1e10 -1e10 -1e10) (1e10 1e10 1e10)"
                });
         setsToZones(*this, dir_);
       }

       std::shared_ptr<OFdicts> createDictionaries() const override
       {
         auto r=SimpleFoamCylinderOpenFOAMCase::createDictionaries();
         r->lookupDict("system/controlDict").getStringRef("application")="porousSimpleFoam";
         return r;
       }

    } tc(argv[1]);

    porousZone::Parameters p;
    p.set_name("allCells");
    tc.insert(new porousZone(tc, p));

    tc.runTest();

  });
}
