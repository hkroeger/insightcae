#include "uniof_tools.h"

#include "fvCFD.H"
#include "Tuple2.H"
#include "surfaceToCell.H"
#include "cellSet.H"
#include "faceSet.H"
#include "cellToFace.H"
#include "OFstream.H"
#include "syncTools.H"


namespace Foam
{



void findBndFaces(const fvMesh& mesh, const cellSet& cells, labelList& res, scalarField& normal_direction)
{
    const label nInt = mesh.nInternalFaces();
    const labelList& own = mesh.faceOwner();
    const labelList& nei = mesh.faceNeighbour();
    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    labelHashSet faces;

    // Add all faces from cell
    forAllConstIter(cellSet, cells, iter)
    {
        const label celli = iter.key();
        const labelList& cFaces = mesh.cells()[celli];

        forAll(cFaces, cFacei)
        {
            faces.insert(cFaces[cFacei]);
        }
    }


    // Check all internal faces
    for (label facei = 0; facei < nInt; facei++)
    {
        if (cells.found(own[facei]) && cells.found(nei[facei]))
        {
            faces.erase(facei);
        }
    }

    // Get coupled cell status
    boolList neiInSet(mesh.nFaces()-nInt, false);

    forAll(patches, patchi)
    {
        const polyPatch& pp = patches[patchi];

        if (pp.coupled())
        {
            label facei = pp.start();
            forAll(pp, i)
            {
                neiInSet[facei-nInt] = cells.found(own[facei]);
                facei++;
            }
        }
    }
    syncTools::swapBoundaryFaceList(mesh, neiInSet
#if (OF_VERSION>=010600 && OF_VERSION<010700)
                                    , false
#endif
                                    );

    // Check all boundary faces
    forAll(patches, patchi)
    {
        const polyPatch& pp = patches[patchi];

        if (pp.coupled())
        {
            label facei = pp.start();
            forAll(pp, i)
            {
                if (cells.found(own[facei]) && neiInSet[facei-nInt])
                {
                    faces.erase(facei);
                }
                facei++;
            }
        }
    }


    res.setSize(faces.size());
    normal_direction.setSize(faces.size());
    label j=0;
    forAllConstIter(labelHashSet, faces, i)
    {
        label fi=i.key();

        res[j]=fi;
        if (cells.found(own[fi]))
        {
            normal_direction[j]=1.0;
        }
        else if (cells.found(nei[fi]))
        {
            normal_direction[j]=-1.0;
        }
        else
        {
            std::cerr << "Häää???"<<std::endl;
        }

        j++;
    }

}


}
