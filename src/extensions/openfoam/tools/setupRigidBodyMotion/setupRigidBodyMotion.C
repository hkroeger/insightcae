/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
     \\/     M anipulation  | Copyright (C) 2016 OpenCFD Ltd.
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
    moveDynamicMesh

Group
    grpMeshManipulationUtilities

Description
    Mesh motion and topological mesh changes utility.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "dynamicFvMesh.H"
#include "pimpleControl.H"
#include "vtkSurfaceWriter.H"
#include "cyclicAMIPolyPatch.H"
#include "PatchTools.H"
#include "dynamicFvMesh.H"
#include "extendedRigidBodyMotion.H"

#include "uniof.h"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
class initDynamicMotionSolverFvMesh
    :public dynamicFvMesh
{

    autoPtr<motionSolver> motionPtr_;

public:
    
    initDynamicMotionSolverFvMesh(const IOobject& io)
        : dynamicFvMesh(io),
          motionPtr_(motionSolver::New(*this))
    {}

    virtual bool update()
    {
        if (auto ems = dynamic_cast<extendedRigidBodyMeshMotion*>(motionPtr_.get()))
        {
            ems->resetPointsFromCurMotionState();
        }
        else
        {
            WarningIn("initDynamicMotionSolverFvMesh::update()")
                    << "could not cast motion solver into extendedRigidBodyMeshMotion: no action!"
                    << endl;
        }

        fvMesh::movePoints(motionPtr_().curPoints());

        return true;
    }

};
    

#if OF_VERSION>=060505
class initDynamicMotionSolverListFvMesh
    :public dynamicFvMesh
{
    PtrList<motionSolver> motionSolvers_;

public:

    initDynamicMotionSolverListFvMesh(const IOobject& io)
        : dynamicFvMesh(io)
    {

        IOobject ioDict
        (
            "dynamicMeshDict",
            time().constant(),
            *this,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
        );

        IOdictionary dict(ioDict);

        label i = 0;
        if (dict.found("solvers"))
        {
            const dictionary& solvertDict = dict.subDict("solvers");

            motionSolvers_.setSize(solvertDict.size());

            for (const entry& dEntry : solvertDict)
            {
                if (dEntry.isDict())
                {
                    IOobject io(ioDict);
                    io.readOpt(IOobject::NO_READ);
                    io.writeOpt(IOobject::AUTO_WRITE);
                    io.rename(dEntry.dict().dictName());

                    IOdictionary IOsolverDict
                    (
                        io,
                        dEntry.dict()
                    );

                    motionSolvers_.set
                    (
                        i++,
                        motionSolver::New(*this, IOsolverDict)
                    );
                }
            }
            motionSolvers_.setSize(i);
        }
    }

    virtual bool update()
    {

        forAll(motionSolvers_, mi)
        {
            if (auto ems = dynamic_cast<extendedRigidBodyMeshMotion*>(&motionSolvers_[mi]))
            {
                ems->resetPointsFromCurMotionState();
            }
            else
            {
                WarningIn("initDynamicMotionSolverListFvMesh::update()")
                        << "could not cast motion solver into extendedRigidBodyMeshMotion: no action!"
                        << endl;
            }
        }

        if (motionSolvers_.size())
        {
            // Accumulated displacement
            pointField disp(motionSolvers_[0].curPoints() - fvMesh::points());

            for (label i = 1; i < motionSolvers_.size(); i++)
            {
                disp += motionSolvers_[i].curPoints() - fvMesh::points();
            }

            fvMesh::movePoints(points() + disp);

            volVectorField* Uptr = getObjectPtr<volVectorField>("U");

            if (Uptr)
            {
                Uptr->correctBoundaryConditions();
            }
        }

        return true;
    }
};
#endif


int main(int argc, char *argv[])
{
    #include "addRegionOption.H"
    #include "setRootCase.H"
    #include "createTime.H"

    Foam::word regionName;

    if (UNIOF_OPTIONREADIFPRESENT(args, "region", regionName))
    {
        Foam::Info
            << "Create mesh " << regionName << " for time = "
            << runTime.timeName() << Foam::nl << Foam::endl;
    }
    else
    {
        regionName = Foam::fvMesh::defaultRegion;
        Foam::Info
            << "Create mesh for time = "
            << runTime.timeName() << Foam::nl << Foam::endl;
    }


    autoPtr<dynamicFvMesh> mesh;

    {
        IOdictionary ioDict(
          IOobject(
            "dynamicMeshDict",
            runTime.constant(),
            runTime,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
          )
        );
        word meshType(ioDict.lookup("dynamicFvMesh"));

#if OF_VERSION>=060505
        if (meshType=="dynamicOversetFvMesh")
        {
            mesh.reset(new initDynamicMotionSolverListFvMesh
                (
                    IOobject
                    (
                        regionName,
                        runTime.timeName(),
                        runTime,
                        IOobject::MUST_READ
                    )
                )
            );
        }
        else
#endif
        {
            mesh.reset(new initDynamicMotionSolverFvMesh
                (
                    IOobject
                    (
                        regionName,
                        runTime.timeName(),
                        runTime,
                        IOobject::MUST_READ
                    )
                )
            );
        }
    }


    Info << "Updating mesh points." <<nl<<endl;
    mesh->update();
    mesh->write();
   

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
