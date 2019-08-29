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

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
class initDynamicMotionSolverFvMesh
    :public dynamicFvMesh
{

    autoPtr<extendedRigidBodyMeshMotion> motionPtr_;

public:
    
    initDynamicMotionSolverFvMesh(const IOobject& io)
        : dynamicFvMesh(io),
          motionPtr_
          (
              new extendedRigidBodyMeshMotion
              (
                  *this,
                  IOdictionary
                  (
                      IOobject
                      (
                          "dynamicMeshDict",
                          this->time().constant(),
                          *this,
                          IOobject::MUST_READ_IF_MODIFIED,
                          IOobject::AUTO_WRITE
                      )
                  )
              )
          )
    {
    }

    virtual bool update()
    {

        fvMesh::movePoints(motionPtr_().curMotionStatePoints());
        return true;
    }

};
    

int main(int argc, char *argv[])
{
    #include "addRegionOption.H"
    #include "setRootCase.H"
    #include "createTime.H"

    Foam::word regionName;

    if (args.optionReadIfPresent("region", regionName))
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


    initDynamicMotionSolverFvMesh mesh
    (
        IOobject
        (
            regionName,
            runTime.timeName(),
            runTime,
            IOobject::MUST_READ
        )
    );



    Info << "Updating mesh points." <<nl<<endl;
    mesh.update();
    mesh.write();
   

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
