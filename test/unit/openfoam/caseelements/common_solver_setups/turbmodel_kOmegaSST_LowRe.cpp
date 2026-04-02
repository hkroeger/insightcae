#include "openfoamcasewithcylindermesh.h"
#include "openfoam/caseelements/turbulencemodels/komegasst_lowre_rasmodel.h"


int main(int argc, char*argv[])
{
    using namespace insight;

    return executeTest([=](){

        insight::assertion(argc==2, "expected exactly one command line argument");

        struct TC
            : public SimpleFoamOpenFOAMCase<OpenFOAMCaseWithBoxMesh<> >
        {
        public:
            using SimpleFoamOpenFOAMCase<OpenFOAMCaseWithBoxMesh<> >
                ::SimpleFoamOpenFOAMCase;

            void createCaseElements() override
            {
                SimpleFoamOpenFOAMCase::createCaseElements();
                this->insert(new kOmegaSST_LowRe_RASModel(*this));
            }
        } tc(argv[1], CaseFeatures{CaseFeature::TurbulenceModel});

        tc.runTest();
    });
}
