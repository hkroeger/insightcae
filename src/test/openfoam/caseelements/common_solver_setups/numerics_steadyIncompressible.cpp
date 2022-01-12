#include "openfoamcasewithcylindermesh.h"


int main(int, char*argv[])
{
    return executeTest([=](){
    SimpleFoamCylinderOpenFOAMCase tc(argv[1]);
    tc.runTest();
    });
}
