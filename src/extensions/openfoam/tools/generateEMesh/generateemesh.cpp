#include "uniof.h"

#include "cadfeatures/importsolidmodel.h"
#include "cademesh.h"

#include "argList.H"
#include "fileName.H"
#include "wordReList.H"


using namespace Foam;

int main(int argc, char* argv[])
{
    argList::noParallel();
    argList::validArgs.append("input geometry file");
    argList::validArgs.append("output eMesh file");
    UNIOF_ADDOPT
    (   argList,
        "tolerance",
        "scalar",
        "tolerance for discretizing the geometry edges"
    );

    argList args(argc, argv);

    fileName inFileName(UNIOF_ADDARG(args,0));
    fileName outFileName(UNIOF_ADDARG(args,1));


    Info<< "Reading geometry from " << inFileName << " ..." << endl;
    auto feat =
           insight::cad::Import::create(
            boost::filesystem::path(inFileName) );
    auto bb=feat->modelBndBox();
    Info<<"bounding box:"<<nl
        <<" min ("<<bb(0,0)<<" "<<bb(1,0)<<" "<<bb(2,0)<<")"<<nl
        <<" max ("<<bb(0,1)<<" "<<bb(1,1)<<" "<<bb(2,1)<<")"<<nl;

    Info<< "Writing eMesh to " << outFileName << " ..." << endl;

    scalar tol=1e-3;
    UNIOF_OPTIONREADIFPRESENT(args, "tolerance", tol);
    Info<< " * discretizing with tolerance "<<tol<<endl;

    insight::cad::CADEMesh emesh(feat, tol);

    emesh.write(outFileName);

    return 0;
}
