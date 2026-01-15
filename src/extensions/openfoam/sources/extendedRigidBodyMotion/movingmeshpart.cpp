
#include "movingmeshpart.h"
#include "cellSet.H"
#include "syncTools.H"


namespace Foam {


movingMeshPart::movingMeshPart(const dictionary &dict, const polyMesh &mesh)
    : mesh_(mesh)
{
    word cellZoneName =
        dict.lookupOrDefault<word>("cellZone", "none");

    word cellSetName =
        dict.lookupOrDefault<word>("cellSet", "none");

    if ((cellZoneName != "none") && (cellSetName != "none"))
    {
        FatalIOErrorInFunction(dict)
            << "Either cellZone OR cellSet can be supplied, but not both. "
            << "If neither is supplied, all cells will be included"
            << exit(FatalIOError);
    }

    labelList cellIDs;
    if (cellZoneName != "none")
    {
        Info<< "Applying solid body motion to cellZone " << cellZoneName
            << endl;

        label zoneID = mesh.cellZones().findZoneID(cellZoneName);

        if (zoneID == -1)
        {
            FatalErrorInFunction
                << "Unable to find cellZone " << cellZoneName
                << ".  Valid cellZones are:"
                << mesh.cellZones().names()
                << exit(FatalError);
        }

        cellIDs = mesh.cellZones()[zoneID];
    }

    if (cellSetName != "none")
    {
        Info<< "Applying solid body motion to cellSet " << cellSetName
            << endl;

        cellSet set(mesh, cellSetName);

        cellIDs = set.toc();
    }

    label nCells = returnReduce(cellIDs.size(), sumOp<label>());

    // collect point IDs of points in cell zone

    boolList movePts(mesh.nPoints(), false);

    forAll(cellIDs, i)
    {
        label celli = cellIDs[i];
        const cell& c = mesh.cells()[celli];
        forAll(c, j)
        {
            const face& f = mesh.faces()[c[j]];
            forAll(f, k)
            {
                label pointi = f[k];
                movePts[pointi] = true;
            }
        }
    }

    syncTools::syncPointList(mesh, movePts, orEqOp<bool>(), false);

    DynamicList<label> ptIDs(mesh.nPoints());
    forAll(movePts, i)
    {
        if (movePts[i])
        {
            ptIDs.append(i);
        }
    }

    pointIDs_.transfer(ptIDs);
}


movingMeshPart::~movingMeshPart()
{}


} // namespace Foam
