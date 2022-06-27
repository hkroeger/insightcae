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
#include "cartesianCS.H"

#include "directforce.h"
#include "patchmomentumforce.h"

#include "extendedForcesFunctionObject.H"
#include "forcesourcecombination.h"
#include "globalcoordinatesystems.h"

#include <memory>
#include <functional>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class extendedRigidBodyMeshMotion Declaration
\*---------------------------------------------------------------------------*/


class PIController
{
    struct Update
    {
        scalar t, e, integral;
    };

    scalar Kp_, TN_;
    scalar integral_;

    std::shared_ptr<Update> lastUpdate_;

public:
    PIController(double Kp, double TN);


    scalar u(scalar t, scalar e);

    void restoreState(const dictionary& id);
    void saveState(dictionary& od) const;
};




class extendedRigidBodyMeshMotion
: public displacementMotionSolver
{

public:


    //- Class containing the patches and point motion weighting for each body
    class bodyMesh
            : public coordinateSystemSource
    {
        const polyMesh& mesh_;

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

        autoPtr<extendedForces> forcesFO_;
        
        vector globalMotionDirection_;

        PtrList<extRBM::additionalForceAndMoment> additionalForces_;

        autoPtr<cartesianCS> currentCoordinateSystem_;

        mutable PtrList<OFstream> addForcesLog_;


    public:

        friend class extendedRigidBodyMeshMotion;

        bodyMesh
        (
            const polyMesh& mesh,
            const word& name,
            const label bodyID,
            const dictionary& dict,
            scalar rhoInf,
            const word& rhoName
        );

        inline const pointScalarField& weight() const
        { return weight_; }

        inline label bodyID() const { return bodyID_; }

        extendedForces& forcesFO();
        const extendedForces& forcesFO() const;

        std::pair<vector,vector> calcForcesMoments(
                bool logForces=false );

//        inline extRBM::directForce& getDirectThrustForce(label i)
//        { return directThrustForces_[i]; };

        void updateCoordinateSystem(const vector& p0, const vector& ez, const vector& ex);

        autoPtr<coordinateSystem> getCoordinateSystem() const override;
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
        label curTimeIndex_; // for RBD updating
        
        vector referenceSystemSpeed_;

        scalar rampDuration_;
        
        mutable autoPtr<OFstream> rbmLogFile_;

    // Private Member Functions

        //- Disallow default bitwise copy construct
        extendedRigidBodyMeshMotion
        (
            const extendedRigidBodyMeshMotion&
        );

        //- Disallow default bitwise assignment
        void operator=(const extendedRigidBodyMeshMotion&);

        void updateBodyMeshCoordinateSystems();

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

        PtrList<bodyMesh>& bodyMeshes();
        const PtrList<bodyMesh>& bodyMeshes() const;
        const RBD::rigidBodyMotion& model() const;
        const RBD::rigidBodyModelState& motionState() const;

        bool isBodyPatch(label patchID) const;


        //- Solve for motion
        void solve() override;

        virtual void populateRigidBodyMotionStateDict(dictionary& dict) const;

        //- Write state using given format, version and compression
        bool writeObject
        (
#if OF_VERSION>=060505
            IOstreamOption streamOpt
#else
            IOstream::streamFormat fmt,
            IOstream::versionNumber ver,
            IOstream::compressionType cmp
#endif
#if OF_VERSION>=060000 //defined(OFesi1806)
            , const bool valid
#endif
        ) const override;

        void continueLogFile() const;

        virtual std::string logFileHeaderLine() const;
        virtual void writeLogLine(Ostream& os) const;

        //- Read dynamicMeshDict dictionary
        bool read() override;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
