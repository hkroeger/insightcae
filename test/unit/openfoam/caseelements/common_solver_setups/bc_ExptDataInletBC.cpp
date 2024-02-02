#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/boundaryconditions/exptdatainletbc.h"

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
          ExptDataInletBC::Parameters::data_type data = {
              { vec3(-1, -1, -1), vec3(1, 0, 0), 0.1, 0.1 },
              { vec3(-1, -1,  1), vec3(1, 0, 0), 0.1, 0.1 },
              { vec3(-1,  0, -1), vec3(2, 0, 0), 0.1, 0.1 },
              { vec3(-1,  0,  1), vec3(2, 0, 0), 0.1, 0.1 },
              { vec3(-1,  1, -1), vec3(1, 0, 0), 0.1, 0.1 },
              { vec3(-1,  1,  1), vec3(1, 0, 0), 0.1, 0.1 }
          };
          insert(new ExptDataInletBC(*this, "inlet", boundaryDict, ExptDataInletBC::Parameters()
                                     .set_data(data)
                                 ));
        }


    } tc(argv[1]);

    tc.runTest();

  });
}
