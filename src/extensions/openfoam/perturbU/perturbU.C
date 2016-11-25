/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2005 OpenCFD Ltd.
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
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

Application
    perturbU

Description
    initialise channel velocity with superimposed streamwise streaks.
    To be used to force channelOodles to transition and reach a fully
    developed flow sooner.

    Reads in perturbUDict.

    EdV from paper:
        Schoppa, W. and Hussain, F.
        "Coherent structure dynamics in near wall turbulence",
        Fluid Dynamics Research, Vol 26, pp119-139, 2000.

\*---------------------------------------------------------------------------*/


#include "fvCFD.H"
#include "Random.H"
#include "wallDist.H"
#include "cuttingPlane.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
   argList::validArgs.append("Retau");
   argList::validArgs.append("Ubulk");
   
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"
   
   Switch resetU(true);
   Switch setPerturb(true);
   
    Info<< "Calculating wall distance field" << endl;
    volScalarField yf(wallDist(mesh).y());
//     yf.write();
    
    // direction towards wall
    volVectorField nh=-fvc::grad(yf);
    nh/=(mag(nh)+dimensionedScalar("", nh.dimensions(), SMALL));
//     nh.rename("grad(y)");
//     nh.write();
    
    scalar Retau=readScalar(IStringStream(
#ifdef OFdev
      args.arg(1)
#else
      args.additionalArgs()[0]
#endif
    )());
    
    vector Ubar=vector(IStringStream(
#ifdef OFdev
      args.arg(2)
#else
      args.additionalArgs()[1]
#endif
    )());
    
    vector nflow=Ubar/mag(Ubar);
    
    volVectorField ntan = nh ^ nflow;

    scalar h=max(yf).value();
    
    Info<< "Channel half height       = " << h << nl
        << "Re(tau)                   = " << Retau << nl
        << "Ubar                      = " << Ubar << nl
        << endl;

    
    Info<< "Reading U" << endl;
    volVectorField U
    (
      IOobject
      (
	  "U",
	  runTime.timeName(),
	  mesh,
	  IOobject::MUST_READ
      ), 
      mesh
    );
    
    point center=boundBox(mesh.C()).midpoint();
#ifdef OF16ext
    cuttingPlane cpl(plane(center, nflow), mesh);
#else
    cuttingPlane cpl(plane(center, nflow), mesh, false);
#endif
    vector Ub=gAverage(cpl.sample(U));
    scalar mUb=mag(Ub);
    scalar fluc=mag(gAverage( sqr(U - dimensionedVector("", dimVelocity, Ub))() ));
    Info<<"mean flow="<<Ub<<", mean fluctuation magnitude="<<fluc<<endl;
    if (mUb>SMALL && (fluc>(1e-3*mUb)))
    {
      scalar sf=mag(Ubar)/mUb;
      Info<<"Mean flow with fluctuations is present, scaling to proper bulk velocity by "<<sf<<endl;
      U*=sf;
      U.write();
      return (0);
    }

    IOdictionary transportProperties
    (
        IOobject
        (
            "transportProperties",
            runTime.constant(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    );

    dimensionedScalar nu
    (
        transportProperties.lookup("nu")
    );


    Info<< "nu      = " << nu << endl;
    Info<< "Ubar    = " << Ubar << endl;
    Info<< "Re(tau) = " << Retau << endl;
    const scalar utau = Retau*nu.value()/h;
    Info<< "u(tau)  = " << utau << endl;


    //wall normal circulation
    const scalar duplus = (nflow&Ubar)*0.25/utau;
    //spanwise wavenumber: spacing z+ = 200
    const scalar betaPlus = 2.0*M_PI*(1.0/200.0);
    const scalar sigma = 0.00055;
    //streamwise wave number: spacing x+ = 500
    const scalar alphaPlus = 2.0*M_PI*(1.0/500.0);
    const scalar epsilon = (nflow&Ubar)/200.0;

    // Random number generator
    Random perturbation(1234567);

    const vectorField& centres = mesh.C();

    forAll(centres, celli)
    {
        // add a small (+/-20%) random component to enhance symetry breaking
        scalar deviation=1.0 + 0.2*perturbation.GaussNormal();

        const vector& cCentre = centres[celli];

        scalar zplus = (ntan[celli]&cCentre)*Retau/h;
        scalar y = yf[celli]; //min(yf[celli], 2*h-(yf[celli]));
        scalar yplus = y*Retau/h;
        scalar xplus = (nflow&cCentre)*Retau/h;

        if (resetU)
        {
            // laminar parabolic profile
            U[celli] = vector::zero;

            U[celli] =
                3.0*Ubar * (y/h - 0.5*sqr(y/h));
        }

        if (setPerturb)
        {
            // streak streamwise velocity
            U[celli] += nflow*
                (utau * duplus/2.0) * (yplus/40.0)
                * Foam::exp(-sigma * Foam::sqr(yplus) + 0.5)
                * Foam::cos(betaPlus*zplus)*deviation;

            // streak spanwise perturbation
            U[celli] += ntan[celli]*
                epsilon
              * Foam::sin(alphaPlus*xplus)
              * yplus
              * Foam::exp(-sigma*Foam::sqr(yplus))
              * deviation;
        }
    }

    Info<< "Writing modified U field to " << runTime.timeName() << endl;
    U.write();

    Info<< endl;

    return(0);
}


// ************************************************************************* //
