#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/turbulencemodels/wale_lesmodel.h"


int main(int argc, char*argv[])
{
    using namespace insight;

    return executeTest([=](){

        insight::assertion(argc==2, "expected exactly one command line argument");

        struct TC
            : public PimpleFoamOpenFOAMCase<OpenFOAMCaseWithBoxMesh<> >
        {
        public:
            using PimpleFoamOpenFOAMCase<OpenFOAMCaseWithBoxMesh<> >
                ::PimpleFoamOpenFOAMCase;

            void createCaseElements() override
            {
                PimpleFoamOpenFOAMCase::createCaseElements();
                this->insert(new WALE_LESModel(*this));
            }
        } tc(argv[1], CaseFeatures{CaseFeature::TurbulenceModel});

        tc.runTest();
    });
}