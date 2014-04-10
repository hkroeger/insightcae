/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Class
    faceQualityMarkerFunctionObject

Description
    FunctionObject checking the mesh

Author
    Hrvoje Jasak, Wikki Ltd.  All rights reserved

SourceFiles
    faceQualityMarkerFunctionObject.C

\*---------------------------------------------------------------------------*/

#ifndef faceQualityMarkerFunctionObject_H
#define faceQualityMarkerFunctionObject_H

#include "fvCFD.H"
#include "functionObject.H"
#include "dictionary.H"
#include "Switch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class faceQualityMarkerFunctionObject Declaration
\*---------------------------------------------------------------------------*/

class faceQualityMarkerFunctionObject
:
    public functionObject
{
    // Private data

        //- Reference to main object registry
        const Time& time_;

        //- Region name
        word regionName_;

        Switch markNonOrthFaces_;
        Switch markSkewFaces_;

        const polyMesh& mesh_;
        autoPtr<surfaceScalarField> UBlendingFactor_;

    // Private Member Functions

        //- Disallow default bitwise copy construct
        faceQualityMarkerFunctionObject(const faceQualityMarkerFunctionObject&);

        //- Disallow default bitwise assignment
        void operator=(const faceQualityMarkerFunctionObject&);

        void updateBlendingFactor();

public:

    //- Runtime type information
    TypeName("faceQualityMarker");


    // Constructors

        //- Construct from components
        faceQualityMarkerFunctionObject
        (
            const word& name,
            const Time&,
            const dictionary&
        );


    // Member Functions

        //- start is called at the start of the time-loop
        virtual bool start();

        //- execute is called at each ++ or += of the time-loop
        virtual bool execute
        (
#ifndef OF16ext
	  bool
#endif
	);

        //- Read and set the function object if its data has changed
        virtual bool read(const dictionary& dict);

#if !defined(OF16ext) && !defined(OF21x)
          //- Update for changes of mesh
        virtual void updateMesh(const mapPolyMesh& mpm);

        //- Update for changes of mesh
        virtual void movePoints(const polyMesh& mesh);
#endif
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
