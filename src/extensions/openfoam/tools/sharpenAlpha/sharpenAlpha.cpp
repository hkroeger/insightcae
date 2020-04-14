#include "fvCFD.H"
#include "uniof.h"

int main(int argc, char *argv[])
{
    argList::validOptions.insert("alphaName", "name of alpha field");

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"



    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    word alphaName = "alpha.phase1";
    if (UNIOF_OPTIONFOUND(args, "alphaName"))
      alphaName = args.options()["alphaName"];

    volScalarField alpha1
            (
                IOobject
                (
                    alphaName,
                    runTime.timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::AUTO_WRITE
                 ),
                mesh
             );

    alpha1 = pos(alpha1-0.5);
    alpha1.write();

    Info<< "End\n" << endl;

    return 0;
}
