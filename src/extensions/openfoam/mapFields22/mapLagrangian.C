/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2012 OpenFOAM Foundation
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

\*---------------------------------------------------------------------------*/

#include "MapLagrangianFields.H"
#if (defined(OF16ext))
#include "passiveParticle.H"
#include "Cloud.H"
#else
#include "passiveParticleCloud.H"
#endif
#include "meshSearch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

static const scalar perturbFactor = 1e-6;

#ifdef OF16ext

// Special version of findCell that generates a cell guaranteed to be
// compatible with tracking.
static label findCell(const meshSearch& meshSearcher, const point& pt)
{
    const polyMesh& mesh = meshSearcher.mesh();

    // Use tracking to find cell containing pt
    label cellI = meshSearcher.findCell(pt);

    if (cellI >= 0)
    {
        return cellI;
    }
    else
    {
        // See if particle on face by finding nearest face and shifting
        // particle.

        label faceI = meshSearcher.findNearestBoundaryFace(pt);

        if (faceI >= 0)
        {
            const point& cc = mesh.cellCentres()[mesh.faceOwner()[faceI]];
            const point perturbPt = (1-perturbFactor)*pt+perturbFactor*cc;

            return meshSearcher.findCell(perturbPt);
        }
    }
    return -1;
}

#else

// Special version of findCell that generates a cell guaranteed to be
// compatible with tracking.
static label findCell(const Cloud<passiveParticle>& cloud, const point& pt)
{
    label cellI = -1;
    label tetFaceI = -1;
    label tetPtI = -1;

    const polyMesh& mesh = cloud.pMesh();

    mesh.findCellFacePt(pt, cellI, tetFaceI, tetPtI);

    if (cellI >= 0)
    {
        return cellI;
    }
    else
    {
        // See if particle on face by finding nearest face and shifting
        // particle.

        meshSearch meshSearcher
        (
            mesh,
            polyMesh::FACEPLANES    // no decomposition needed
        );

        label faceI = meshSearcher.findNearestBoundaryFace(pt);

        if (faceI >= 0)
        {
            const point& cc = mesh.cellCentres()[mesh.faceOwner()[faceI]];

            const point perturbPt = (1-perturbFactor)*pt+perturbFactor*cc;

            mesh.findCellFacePt(perturbPt, cellI, tetFaceI, tetPtI);

            return cellI;
        }
    }

    return -1;
}

#endif


void mapLagrangian(const meshToMesh& meshToMeshInterp)
{
    // Determine which particles are in meshTarget
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // target to source cell map
    const labelList& cellAddressing = meshToMeshInterp.cellAddressing();

    // Invert celladdressing to get source to target(s).
    // Note: could use sparse addressing but that is too storage inefficient
    // (Map<labelList>)
    labelListList sourceToTargets
    (
        invertOneToMany(meshToMeshInterp.fromMesh().nCells(), cellAddressing)
    );

    const fvMesh& meshSource = meshToMeshInterp.fromMesh();
    const fvMesh& meshTarget = meshToMeshInterp.toMesh();
    const pointField& targetCc = meshTarget.cellCentres();


    fileNameList cloudDirs
    (
        readDir
        (
            meshSource.time().timePath()/cloud::prefix,
            fileName::DIRECTORY
        )
    );

    forAll(cloudDirs, cloudI)
    {
        // Search for list of lagrangian objects for this time
        IOobjectList objects
        (
            meshSource,
            meshSource.time().timeName(),
            cloud::prefix/cloudDirs[cloudI]
        );

        IOobject* positionsPtr = objects.lookup("positions");

        if (positionsPtr)
        {
            Info<< nl << "    processing cloud " << cloudDirs[cloudI] << endl;

            // Read positions & cell
#ifdef OF16ext
	    Cloud<passiveParticle>
#else
            passiveParticleCloud 
#endif
            sourceParcels
            (
                meshSource,
                cloudDirs[cloudI],
                false
            );
            Info<< "    read " << sourceParcels.size()
                << " parcels from source mesh." << endl;

            // Construct empty target cloud
#ifdef OF16ext
	    Cloud<passiveParticle>
#else
            passiveParticleCloud 
#endif
            targetParcels
            (
                meshTarget,
                cloudDirs[cloudI],
                IDLList<passiveParticle>()
            );

#ifndef OF16ext
            particle::TrackingData<passiveParticleCloud> td(targetParcels);
#endif

            label sourceParticleI = 0;

            // Indices of source particles that get added to targetParcels
            DynamicList<label> addParticles(sourceParcels.size());

            // Unmapped particles
            labelHashSet unmappedSource(sourceParcels.size());


            // Initial: track from fine-mesh cell centre to particle position
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            // This requires there to be no boundary in the way.


            forAllConstIter(Cloud<passiveParticle>, sourceParcels, iter)
            {
                bool foundCell = false;

                // Assume that cell from read parcel is the correct one...
                if (iter().cell() >= 0)
                {
                    const labelList& targetCells =
                        sourceToTargets[iter().cell()];

                    // Particle probably in one of the targetcells. Try
                    // all by tracking from their cell centre to the parcel
                    // position.

                    forAll(targetCells, i)
                    {
                        // Track from its cellcentre to position to make sure.
                        autoPtr<passiveParticle> newPtr
                        (
                            new passiveParticle
                            (
                                meshTarget,
                                targetCc[targetCells[i]],
                                targetCells[i]
                            )
                        );
                        passiveParticle& newP = newPtr();

			scalar fraction = 0;
                        label faceI = newP.track(iter().position(), 
#ifdef OF16ext
						 fraction
#else
						 td
#endif
						);

                        if (faceI < 0 && newP.cell() >= 0)
                        {
                            // Hit position.
                            foundCell = true;
                            addParticles.append(sourceParticleI);
                            targetParcels.addParticle(newPtr.ptr());
                            break;
                        }
                    }
                }

                if (!foundCell)
                {
                    // Store for closer analysis
                    unmappedSource.insert(sourceParticleI);
                }

                sourceParticleI++;
            }

            Info<< "    after meshToMesh addressing found "
                << targetParcels.size()
                << " parcels in target mesh." << endl;


            // Do closer inspection for unmapped particles
            // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

            if (unmappedSource.size())
            {
#ifdef OF16ext
		meshSearch targetSearcher(meshTarget, false);
#endif
                sourceParticleI = 0;

                forAllIter(Cloud<passiveParticle>, sourceParcels, iter)
                {
                    if (unmappedSource.found(sourceParticleI))
                    {
                        label targetCell =
                            findCell(
#ifdef OF16ext
			      targetSearcher
#else
			      targetParcels
#endif
			      , iter().position());

                        if (targetCell >= 0)
                        {
                            unmappedSource.erase(sourceParticleI);
                            addParticles.append(sourceParticleI);
                            iter().cell() = targetCell;
                            targetParcels.addParticle
                            (
                                sourceParcels.remove(&iter())
                            );
                        }
                    }
                    sourceParticleI++;
                }
            }
            addParticles.shrink();

            Info<< "    after additional mesh searching found "
                << targetParcels.size() << " parcels in target mesh." << endl;

            if (addParticles.size())
            {
                IOPosition<
#ifdef OF16ext
	    passiveParticle
#else
            passiveParticleCloud 
#endif
                >(targetParcels).write();

                // addParticles now contains the indices of the sourceMesh
                // particles that were appended to the target mesh.

                // Map lagrangian fields
                // ~~~~~~~~~~~~~~~~~~~~~

                MapLagrangianFields<label>
                (cloudDirs[cloudI], objects, meshToMeshInterp, addParticles);
                MapLagrangianFields<scalar>
                (cloudDirs[cloudI], objects, meshToMeshInterp, addParticles);
                MapLagrangianFields<vector>
                (cloudDirs[cloudI], objects, meshToMeshInterp, addParticles);
                MapLagrangianFields<sphericalTensor>
                (cloudDirs[cloudI], objects, meshToMeshInterp, addParticles);
                MapLagrangianFields<symmTensor>
                (cloudDirs[cloudI], objects, meshToMeshInterp, addParticles);
                MapLagrangianFields<tensor>
                (cloudDirs[cloudI], objects, meshToMeshInterp, addParticles);
            }
        }
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
