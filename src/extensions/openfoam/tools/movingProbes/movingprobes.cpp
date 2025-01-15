
#include "fvCFD.H"
#include "uniof.h"

#include "meshSearch.H"
#include "cloudSet.H"

using namespace Foam;


int main(int argc, char *argv[])
{
    timeSelector::addOptions();

    argList::validArgs.append("points");
    argList::validArgs.append("velocity");
    argList::validArgs.append("output file");

#include "setRootCase.H"
#include "createTime.H"
    instantList timeDirs = timeSelector::select0(runTime, args);
#include "createNamedMesh.H"


    pointField points(
        IStringStream(UNIOF_ADDARG(args,0))()
        );

    vector U(
        IStringStream(UNIOF_ADDARG(args,1))()
        );

    fileName outfilename(UNIOF_ADDARG(args,2));

    OFstream f(outfilename);

    f<<"# t";
    forAll(points, pi)
    {
        f<<" pt"<<pi<<"@"<<points[pi];
    }
    f<<endl;

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();

        IOobject pheader
            (
                "p",
                runTime.timeName(),
                mesh,
                IOobject::MUST_READ,
                IOobject::NO_WRITE
                );
        volScalarField p(pheader, mesh);

        auto curPts = points + U*runTime.value();

        f<<runTime.value();
        forAll(curPts(), pi)
        {
            auto ci=mesh.findNearestCell(curPts()[pi]);
            auto pc = p[ci];
            f<<" "<<pc;
        }
        f<<endl;


    }

    Info<< "End\n" << endl;

    return 0;
}
