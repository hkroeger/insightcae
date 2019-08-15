/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
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

#ifndef MapVolFields_H
#define MapVolFields_H

#include "GeometricField.H"
#include "meshToMesh.H"
#include "IOobjectList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

template<class Type, class CombineOp>
void MapVolFields
(
    const IOobjectList& objects,
    const meshToMesh& meshToMeshInterp,
    const meshToMesh::order& mapOrder,
    const CombineOp& cop
)
{
    const fvMesh& meshSource = meshToMeshInterp.fromMesh();
    const fvMesh& meshTarget = meshToMeshInterp.toMesh();

    word fieldClassName
    (
        GeometricField<Type, fvPatchField, volMesh>::typeName
    );

    IOobjectList fields = objects.lookupClass(fieldClassName);

    forAllIter(IOobjectList, fields, fieldIter)
    {
        IOobject fieldTargetIOobject
        (
            fieldIter()->name(),
            meshTarget.time().timeName(),
            meshTarget,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        );

        if (fieldTargetIOobject.headerOk())
        {
            Info<< "    interpolating " << fieldIter()->name()
                << endl;

            // Read field fieldSource
            GeometricField<Type, fvPatchField, volMesh> fieldSource
            (
                *fieldIter(),
                meshSource
            );

            // Read fieldTarget
            GeometricField<Type, fvPatchField, volMesh> fieldTarget
            (
                fieldTargetIOobject,
                meshTarget
            );

            // Interpolate field
            meshToMeshInterp.interpolate//<Type, eqOp<Type> >
            (
                fieldTarget,
                fieldSource,
                mapOrder,
                cop
            );

            // Write field
            fieldTarget.write();
        }
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
