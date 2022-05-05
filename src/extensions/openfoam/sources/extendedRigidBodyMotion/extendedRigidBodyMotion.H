/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2016 OpenFOAM Foundation
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

Class
    Foam::extendedRigidBodyMeshMotion

Group
    grpMeshMotionSolvers 

Description
    Rigid-body mesh motion solver for fvMesh.

    Applies septernion interpolation of movement as function of distance to the
    object surface.

SourceFiles
    extendedRigidBodyMeshMotion.C

\*---------------------------------------------------------------------------*/

#ifndef extendedRigidBodyMeshMotion_H
#define extendedRigidBodyMeshMotion_H

#include "displacementMotionSolver.H"
#include "rigidBodyMotion.H"
#include "OFstream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class extendedRigidBodyMeshMotion Declaration
\*---------------------------------------------------------------------------*/

class extendedRigidBodyMeshMotion
:
    public displacementMotionSolver
{
    class directThrustForce {
    public:
        point PoA_;
        vector direction_;
        scalar resistance_fraction_;
        
        static autoPtr<directThrustForce> New(Istream& is);
        
        directThrustForce(const point&, const vector&, scalar);
        autoPtr<directThrustForce> clone() const;
    };
    
    //- Class containing the patches and point motion weighting for each body
    class bodyMesh
    {
        //- Name of the body
        const word name_;

        //- ID of the body in the RBD::rigidBodyMotion
        const label bodyID_;

        //- List of mesh patches associated with this body
        const wordReList patches_;

        //- Patches to integrate forces
        const labelHashSet patchSet_;

        //- Inner morphing distance (limit of solid-body region)
        const scalar di_;

        //- Outer morphing distance (limit of linear interpolation region)
        const scalar do_;

        //- Current interpolation weight
        //  (1 at patches and within di_, 0 at do_ and beyond)
        pointScalarField weight_;
        
        vector globalMotionDirection_;
        PtrList<directThrustForce> directThrustForces_;

    public:

        friend class extendedRigidBodyMeshMotion;

        bodyMesh
        (
            const polyMesh& mesh,
            const word& name,
            const label bodyID,
            const dictionary& dict
        );

        inline const pointScalarField& weight() const
        { return weight_; }
    };


    // Private data

        //- Rigid-body model
        RBD::rigidBodyMotion model_;

        //- List of the bodyMeshes containing the patches and point motion
        //  weighting for each body
        PtrList<bodyMesh> bodyMeshes_;

        //- Switch for test-mode in which only the
        //  gravitational body-force is applied
        Switch test_;

        //- Reference density required by the forces object for
        //  incompressible calculations, required if rho == rhoInf
        scalar rhoInf_;

        //- Name of density field, optional unless used for an
        //  incompressible simulation, when this needs to be specified
        //  as rhoInf
        word rhoName_;

        //- Current time index (used for updating)
        label curTimeIndex_;
        
        vector referenceSystemSpeed_;

        scalar rampDuration_;
        
        autoPtr<OFstream> filePtr_;

    // Private Member Functions

        //- Disallow default bitwise copy construct
        extendedRigidBodyMeshMotion
        (
            const extendedRigidBodyMeshMotion&
        );

        //- Disallow default bitwise assignment
        void operator=(const extendedRigidBodyMeshMotion&);


public:

    //- Runtime type information
    TypeName("extendedRigidBodyMotion");


    // Constructors

        //- Construct from polyMesh and IOdictionary
        extendedRigidBodyMeshMotion
        (
            const polyMesh&,
            const IOdictionary& dict
        );


    //- Destructor
    ~extendedRigidBodyMeshMotion();


    // Member Functions

        //- Return point location obtained from the current motion field
        tmp<pointField> curPoints() const override;

        tmp<pointField> curMotionStatePoints() const;

        const PtrList<bodyMesh>& bodyMeshes() const;
        const RBD::rigidBodyMotion& model() const;
        const RBD::rigidBodyModelState& motionState() const;

        //- Solve for motion
        void solve() override;

        //- Write state using given format, version and compression
        bool writeObject
        (
            IOstream::streamFormat fmt,
            IOstream::versionNumber ver,
            IOstream::compressionType cmp
#if OF_VERSION>=060000 //defined(OFesi1806)
            , const bool valid
#endif
        ) const override;

        virtual void writeLogLine(Ostream& os) const;

        //- Read dynamicMeshDict dictionary
        bool read() override;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
