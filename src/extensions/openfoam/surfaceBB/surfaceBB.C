#include "triangle.H"
#include "triSurface.H"
#include "triSurfaceSearch.H"
#include "triSurfaceTools.H"
#include "argList.H"
#include "OFstream.H"
#include "OBJstream.H"
#include "SortableList.H"
#include "PatchTools.H"
#include "vtkSurfaceWriter.H"

#include "uniof.h"

using namespace Foam;

int main(int argc, char *argv[])
{
    argList::noParallel();
    argList::validArgs.append("surfaceFile");
    argList args(argc, argv);

    const fileName surfFileName = UNIOF_ADDARG(args, 0);

    Info<< "Reading surface from " << surfFileName << " ..." << nl << endl;

    // Read
    // ~~~~

    triSurface surf(surfFileName);

    Info<< "Statistics:" << endl;
    surf.writeStats(Info);
    Info<< endl;

    return 0;
}
