/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    refineWallLayer

Description
    Utility to refine cells next to patches.

    Takes a patchName and number of layers to refine. Works out cells within
    these layers and refines those in the wall-normal direction.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#if (defined(Fx32)||defined(Fx40)||defined(Fx41))
#include "foamTime.H"
#else
#include "Time.H"
#endif
#if defined(OF16ext)
#include "directTopoChange.H"
#else
#include "polyTopoChange.H"
#include "polyTopoChanger.H"
#endif
#include "mapPolyMesh.H"
#include "polyMesh.H"
#include "cellCuts.H"
#include "cellSet.H"
#include "meshCutter.H"

#include "uniof.h"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    Foam::argList::validOptions.insert("overwrite", "");
    argList::noParallel();
    argList::validArgs.append("patchName");
    argList::validArgs.append("wallDist");

    Foam::argList::validOptions.insert("useSet", "cellSet");


#   include "setRootCase.H"
#   include "createTime.H"
    runTime.functionObjects().off();
#   include "createPolyMesh.H"
    const word oldInstance = mesh.pointsInstance();

//     const word patchName = args[1];
//     const scalar dist  = args.argRead<scalar>(2);
//     const bool overwrite = args.optionFound("overwrite");
    word patchName( UNIOF_ADDARG(args, 0) );

    scalarList dists(IStringStream( UNIOF_ADDARG(args, 1) )());
    bool overwrite = UNIOF_OPTIONFOUND(args, "overwrite");

    label patchID = mesh.boundaryMesh().findPatchID(patchName);

    if (patchID == -1)
    {
        FatalErrorIn(args.executable())
            << "Cannot find patch " << patchName << endl
            << "Valid patches are " << mesh.boundaryMesh().names()
            << exit(FatalError);
    }
    const polyPatch& pp = mesh.boundaryMesh()[patchID];


    forAll(dists, di)
    {
      scalar dist=dists[di];
    // Cells cut

    labelHashSet cutCells(4*pp.size());

    const labelList& meshPoints = pp.meshPoints();

    forAll(meshPoints, pointI)
    {
        label meshPointI = meshPoints[pointI];

        const labelList& pCells = mesh.pointCells()[meshPointI];

        forAll(pCells, pCellI)
        {
            cutCells.insert(pCells[pCellI]);
        }
    }

    Info<< "Selected " << cutCells.size()
        << " cells connected to patch " << pp.name() << endl << endl;

    //
    // List of cells to refine
    //

    word setName;
    if (UNIOF_OPTIONREADIFPRESENT(args, "useSet", setName))
    {
        Info<< "Subsetting cells to cut based on cellSet"
            << setName << nl << endl;

        cellSet cells(mesh, setName);

        Info<< "Read " << cells.size() << " cells from cellSet "
            << cells.instance()/cells.local()/cells.name()
            << nl << endl;

        forAllConstIter(cellSet, cells, iter)
        {
            cutCells.erase(iter.key());
        }
        Info<< "Removed from cells to cut all the ones not in set "
            << setName << nl << endl;
    }

    // Mark all meshpoints on patch

    boolList vertOnPatch(mesh.nPoints(), false);

    forAll(meshPoints, pointI)
    {
        const label meshPointI = meshPoints[pointI];

        vertOnPatch[meshPointI] = true;
    }


    // Mark cut edges.

    DynamicList<label> allCutEdges(pp.nEdges());

    DynamicList<scalar> allCutEdgeWeights(pp.nEdges());
    
    label limited=0;

    forAll(meshPoints, pointI)
    {
        label meshPointI = meshPoints[pointI];

        const labelList& pEdges = mesh.pointEdges()[meshPointI];

        forAll(pEdges, pEdgeI)
        {
            const label edgeI = pEdges[pEdgeI];
            const edge& e = mesh.edges()[edgeI];

            label otherPointI = e.otherVertex(meshPointI);

            if (!vertOnPatch[otherPointI])
            {
                allCutEdges.append(edgeI);
		
		scalar weight= dist/e.mag(mesh.points());
		
		if (weight>0.8)
		{
		  weight=0.8;
		  limited++;
		}

                if (e.start() == meshPointI)
                {
                    allCutEdgeWeights.append(weight);
                }
                else
                {
                    allCutEdgeWeights.append(1 - weight);
                }
            }
        }
    }
    
    if (limited>0)
    {
      Info<<"Limited weight to 0.8 on "<<limited<<" edges."<<endl;
    }

    allCutEdges.shrink();
    allCutEdgeWeights.shrink();

    Info<< "Cutting:" << nl
        << "    cells:" << cutCells.size() << nl
        << "    edges:" << allCutEdges.size() << nl
        << endl;

    // Transfer DynamicLists to straight ones.
    scalarField cutEdgeWeights;
    cutEdgeWeights.transfer(allCutEdgeWeights);
    allCutEdgeWeights.clear();


    // Gets cuts across cells from cuts through edges.
    cellCuts cuts
    (
        mesh,
        cutCells.toc(),     // cells candidate for cutting
        labelList(0),       // cut vertices
        allCutEdges,        // cut edges
        cutEdgeWeights      // weight on cut edges
    );

#if (OF_VERSION<010700) //defined(OF16ext)
    directTopoChange meshMod(mesh);
#else
    polyTopoChange meshMod(mesh);
#endif

    // Cutting engine
    meshCutter cutter(mesh);

    // Insert mesh refinement into polyTopoChange.
    cutter.setRefinement(cuts, meshMod);

    // Do all changes
    Info<< "Morphing ..." << endl;

    if (!overwrite)
    {
        runTime++;
    }

    autoPtr<mapPolyMesh> morphMap = meshMod.changeMesh(mesh, false);

    if (morphMap().hasMotionPoints())
    {
        mesh.movePoints(morphMap().preMotionPoints());
    }

    // Update stored labels on meshCutter.
    cutter.updateMesh(morphMap());

    if (overwrite)
    {
        mesh.setInstance(oldInstance);
    }

    // Write resulting mesh
    Info<< "Writing refined morphMesh to time " << runTime.timeName() << endl;

    mesh.write();
    
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
