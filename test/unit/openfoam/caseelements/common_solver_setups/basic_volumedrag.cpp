#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/basic/volumedrag.h"
#include "openfoam/openfoamtools.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public CyclicPimpleFoamOpenFOAMCase
    {
    public:
      Case(const std::string& ofe) : CyclicPimpleFoamOpenFOAMCase(ofe) {}

      void createMesh() override
      {
        CyclicPimpleFoamOpenFOAMCase::createMesh();
        setSet(*this, dir_,
               {
                 "cellSet drag new cylinderToCell (0 0 0.25) (0 0 0.75) 0.25"
               }
               );
        setsToZones(*this, dir_);
      }
    } tc(argv[1]);

    volumeDrag::Parameters p;
    p.set_name("drag");
    p.set_CD(vec3(1,1,1));
    tc.insert(new volumeDrag(tc, p));

    tc.runTest();

  });
}
