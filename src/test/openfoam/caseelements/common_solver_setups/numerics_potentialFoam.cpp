#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/potentialfoamnumerics.h"

int main(int argc, char*argv[])
{
    return executeTest([=](){

        assertion( argc==2, "Exactly one command line arguments are required!");

        class Case : public OpenFOAMCaseWithCylinderMesh
        {

        public:
            Case(const std::string& ofe) : OpenFOAMCaseWithCylinderMesh(ofe) {}

            void createCaseElements() override
            {
                potentialFoamNumerics::Parameters p;
                insert(new potentialFoamNumerics(*this, p));

                addField("p", FieldInfo(scalarField, 	dimKinPressure, FieldValue({0}), volField ) );
            }

        } cm(argv[1]);


        cm.runTest();
    });
}
