/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2014 OpenFOAM Foundation
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
    applyBoundaryLayer

Description
    Apply a simplified boundary-layer model to the velocity and
    turbulence fields based on the 1/7th power-law.

    The uniform boundary-layer thickness is either provided via the -ybl option
    or calculated as the average of the distance to the wall scaled with
    the thickness coefficient supplied via the option -Cbl.  If both options
    are provided -ybl is used.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "singlePhaseTransportModel.H"

#if (OF_VERSION>=030000) //defined(OF301)||defined(OFplus)||defined(OFdev)||defined(OFesi1806)
#include "turbulentTransportModel.H"
#else
#include "RASModel.H"
#endif

#include "wallDist.H"
#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// turbulence constants - file-scope
static const scalar Cmu(0.09);
static const scalar kappa(0.41);


int main(int argc, char *argv[])
{
    argList::validOptions.insert("ybl", "specify the boundary-layer thickness");
    argList::validOptions.insert("Cbl", "boundary-layer thickness as Cbl * mean distance to wall");
    argList::validOptions.insert("writenut", "");

    #include "setRootCase.H"

    if (!UNIOF_OPTIONFOUND(args, "ybl") && !UNIOF_OPTIONFOUND(args, "Cbl"))
    {
        FatalErrorIn(args.executable())
            << "Neither option 'ybl' or 'Cbl' have been provided to calculate "
            << "the boundary-layer thickness.\n"
            << "Please choose either 'ybl' OR 'Cbl'."
            << exit(FatalError);
    }
    else if (UNIOF_OPTIONFOUND(args, "ybl") && UNIOF_OPTIONFOUND(args, "Cbl"))
    {
        FatalErrorIn(args.executable())
            << "Both 'ybl' and 'Cbl' have been provided to calculate "
            << "the boundary-layer thickness.\n"
            << "Please choose either 'ybl' OR 'Cbl'."
            << exit(FatalError);
    }

    #include "createTime.H"
    #include "createMesh.H"
    #include "createFields.H"
    
    Info<< "Reading/calculating face flux field phi\n" << endl;

    autoPtr<surfaceScalarField> phiPtr
    (
      new surfaceScalarField
      (
	  IOobject
	  (
	      "phi",
	      runTime.timeName(),
	      mesh,
	      IOobject::READ_IF_PRESENT,
	      IOobject::AUTO_WRITE
	  ),
	  linearInterpolate(U) & mesh.Sf()
      )
    );

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // Modify velocity by applying a 1/7th power law boundary-layer
    // u/U0 = (y/ybl)^(1/7)
    // assumes U0 is the same as the current cell velocity

    Info<< "Setting boundary layer velocity" << nl << endl;
    scalar yblv = ybl.value();
    /*
    forAll(U, cellI)
    {
        if (y[cellI] <= yblv)
        {
            mask[cellI] = 1;
            U[cellI] *= ::pow(y[cellI]/yblv, (1.0/7.0));
        }
    }
    mask.correctBoundaryConditions();
    */
    U.correctBoundaryConditions();
    
    forAll(U.boundaryField(), pi)
    {
        const fvPatchVectorField& Up=U.boundaryField()[pi]; //UNIOF_BOUNDARY_NONCONST(U)[pi];
        const labelList& fcs=Up.patch().faceCells();
        forAll(fcs, fi)
        {
            label ci=fcs[fi];
            vector deltaU=U[ci]-Up[fi];
            deltaU *= ::pow(y[ci]/yblv, (1.0/7.0));
            U[ci]=Up[fi]+deltaU;
            mask[ci] = 1;
        }
    }

    Info<< "Writing U\n" << endl;
    U.write();
    
    phiPtr.clear();

    // Update/re-write phi
    #include "createPhi.H"
    phi.write();

    singlePhaseTransportModel laminarTransport(U, phi);

    autoPtr<incompressible::turbulenceModel> turbulence
    (
        incompressible::turbulenceModel::New(U, phi, laminarTransport)
    );

    if (isA<incompressible::RASModel>(turbulence()))
    {
        // Calculate nut - reference nut is calculated by the turbulence model
        // on its construction
        volScalarField nut( turbulence->nut() );
        //volScalarField& nut = UNIOF_TMP_NONCONST(tnut);
        volScalarField S(mag(dev(symm(fvc::grad(U)))));
        nut = (1 - mask)*nut + max(min(nut), mask*sqr(kappa*min(y, ybl))*::sqrt(2)*S);

        // do not correct BC - wall functions will 'undo' manipulation above
        // by using nut from turbulence model

        if (UNIOF_OPTIONFOUND(args, "writenut"))
        {
            Info<< "Writing nut" << endl;
            nut.write();
        }


        //--- Read and modify turbulence fields

        // Turbulence k
        volScalarField k( turbulence->k() );
        //volScalarField& k = UNIOF_TMP_NONCONST(tk);
        
        scalar ck0 = ::pow(Cmu, 0.25)*kappa;
        k = (1 - mask)*k + max(min(k), mask*sqr(nut/(ck0*min(y, ybl))));

        // do not correct BC - operation may use inconsistent fields wrt these
        // local manipulations
        // k.correctBoundaryConditions();

        Info<< "Writing k\n" << endl;
        k.write();


        // Turbulence epsilon
        volScalarField epsilon( turbulence->epsilon() );
        //volScalarField& epsilon = UNIOF_TMP_NONCONST(tepsilon);
        scalar ce0 = ::pow(Cmu, 0.75)/kappa;

        epsilon = (1 - mask)*epsilon + max(min(epsilon), mask*ce0*k*sqrt(k)/min(y, ybl));

        // do not correct BC - wall functions will use non-updated k from
        // turbulence model
        // epsilon.correctBoundaryConditions();

        Info<< "Writing epsilon\n" << endl;
        epsilon.write();

        // Turbulence omega
        IOobject omegaHeader
        (
            "omega",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
        );

        if (UNIOF_HEADEROK(omegaHeader,volScalarField))
        {
            volScalarField omega(omegaHeader, mesh);
            dimensionedScalar k0("VSMALL", k.dimensions(), VSMALL);

            omega = (1 - mask)*omega + max(min(omega), mask*epsilon/(Cmu*k + k0));

            // do not correct BC - wall functions will use non-updated k from
            // turbulence model
            // omega.correctBoundaryConditions();

            Info<< "Writing omega\n" << endl;
            omega.write();

        }

        // Turbulence nuTilda
        IOobject nuTildaHeader
        (
            "nuTilda",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
        );

        if (UNIOF_HEADEROK(nuTildaHeader,volScalarField))
        {
            volScalarField nuTilda(nuTildaHeader, mesh);
            nuTilda = nut;

            // do not correct BC
            // nuTilda.correctBoundaryConditions();

            Info<< "Writing nuTilda\n" << endl;
            nuTilda.write();
        }
    }

    Info<< nl << "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        << nl << endl;

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
