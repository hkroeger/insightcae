/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     | Version:     4.1
    \\  /    A nd           | Web:         http://www.foam-extend.org
     \\/     M anipulation  | For copyright notice see file Copyright
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

Application
    sonicFoam

Description
    Transient solver for trans-sonic/supersonic for laminar or turbulent
    flow of a compressible gas.

    Uses the flexible PIMPLE (PISO-SIMPLE) solution for time-resolved and
    pseudo-transient simulations.  The pressure-energy coupling is done
    using the Rusche manoeuvre (isentropic compression/expansion).

    Turbulence modelling is generic, i.e. laminar, RAS or LES may be selected.

Author
    Hrvoje Jasak, Wikki Ltd.  All rights reserved.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "basicPsiThermo.H"
#include "compressible/turbulenceModel/turbulenceModel.H"
#include "MRFZones.H"
#if OF_VERSION>=010604
#include "pimpleControl.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


#define CHECKPF(FLD) \
{\
  Info << #FLD " min / max / avg = "<<min(FLD).value() << " / " << max(FLD).value() << " / "<<average(FLD).value() << endl;\
}

#define CHECK(FLD) \
{\
  volScalarField FLD = thermo.FLD();\
  Info << #FLD " min / max / avg = "<<min(FLD).value() << " / " << max(FLD).value() << " / "<<average(FLD).value() << endl;\
}

#define CHECKALL \
  CHECKPF(rho) \
  CHECK(T) \
  CHECK(e) \
  CHECK(psi) \
  CHECK(Cp) \
  CHECK(Cv) \

int main(int argc, char *argv[])
{
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

#if OF_VERSION>=010604
    pimpleControl pimple(mesh);
#endif

#   include "createFields.H"
#   include "initContinuityErrs.H"

#if OF_VERSION>=010604
#   include "createTimeControls.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.run())
    {
#       include "readTimeControls.H"
#if OF_VERSION<010604
#       include "readPIMPLEControls.H"
#endif
#       include "compressibleCourantNo.H"
#       include "setDeltaT.H"

        runTime++;
        Info<< "deltaT = " << runTime.deltaT().value() << nl << endl;
        Info<< "Time = " << runTime.timeName() << nl << endl;

        // --- PIMPLE loop
#if OF_VERSION>=010604
        while (pimple.loop())
#else
        label oCorr = 0;
        do
#endif
        {
#           include "rhoEqn.H"
#           include "eEqn.H"
#           include "UEqn.H"

            // --- PISO loop
            volScalarField rUA = 1.0/UEqn.A();

            surfaceScalarField psisf = fvc::interpolate(psis);
            surfaceScalarField rhof = fvc::interpolate(rho);

            // Needs to be outside of loop since p is changing,
            // but psi and rho are not
            surfaceScalarField rhoReff = rhof - psisf*fvc::interpolate(p);

#if OF_VERSION>=010604
            while (pimple.correct())
#else
            for (int corr = 0; corr < nCorr; corr++)
#endif
            {
#               include "pEqn.H"
            }

            // Calculate relative velocity
            Urel == U;
            mrfZones.relativeVelocity(Urel);
           
            turbulence->correct();

            CHECKALL
        }
#if OF_VERSION<010604
        while (++oCorr < nOuterCorr);
#endif

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return(0);
}


// ************************************************************************* //
