#include "openfoamcasewithcylindermesh.h"


int main(int, char*argv[])
{
    return executeTest([=](){
    SimpleFoamOpenFOAMCase tc(argv[1]);
    tc.runTest();
    });
}
