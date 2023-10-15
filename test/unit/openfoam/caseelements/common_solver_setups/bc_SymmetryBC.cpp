#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"

using namespace insight;

int main(int argc, char*argv[])
{
  return executeTest([=](){

    insight::assertion(argc==2, "expected exactly one command line argument");

    class Case : public OpenFOAMCaseWithBoxMesh
    {
    public:
        Case(const std::string& ofe) : OpenFOAMCaseWithBoxMesh(ofe) {}

        void createWallBC(OFDictData::dict& bd) override
        {
            insert(new SymmetryBC(*this, "front", bd));
            insert(new SymmetryBC(*this, "back", bd));
        }

    } tc(argv[1]);

    tc.runTest();

  });
}
