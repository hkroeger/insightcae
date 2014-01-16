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

Application
    yPlusRAS

Description
    Calculates and reports yPlus for all wall patches, for the specified times.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "OFstream.H"
#include "transformField.H"
#include "transformGeometricField.H"

#include "transformation.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;
using namespace insight;


int main(int argc, char *argv[])
{
    timeSelector::addOptions();

    argList::validArgs.append("p0");
    argList::validArgs.append("rho");
    /*
    argList::validOptions.insert("p0", "reference pressure");
    argList::validOptions.insert("noShiftY", "");
    argList::validOptions.insert("writeVTK", "");
    */

#   include "setRootCase.H"
#   include "createTime.H"
    
    instantList timeDirs = timeSelector::select0(runTime, args);
    
#   include "createMesh.H"

    dimensionedScalar p0("p0", dimPressure, 1e5);
    p0.value()=readScalar(IStringStream(args.additionalArgs()[0])());
    
    dimensionedScalar rho("rho", dimDensity, 1e5);
    rho.value()=readScalar(IStringStream(args.additionalArgs()[1])());
    
    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();

        Info << "Reading field p\n" << endl;
        volScalarField p
        (
            IOobject
            (
                "p",
                runTime.timeName(),
                mesh,
                IOobject::MUST_READ,
                IOobject::AUTO_WRITE
            ),
            mesh
        );

	if ( p.dimensions() == dimensionSet(0, 2, -2, 0, 0) )
	{
	  // need to be converted into normalized pressure
	  Info<<"Converting values into normalized pressure values."<<endl;
	  p/=rho.value();
	  p-=p0/rho;
	}
	else if ( p.dimensions() == dimensionSet(1, -1, -2, 0, 0) )
	{
	  Info<<"Converting values into real pressure values."<<endl;
	  // need to be converted into real pressure
	  p*=rho.value();
	  p+=p0;
	}
	else
	  FatalErrorIn("main")
	  << "Don't know how the handle these pressure dimensions: "<<p.dimensions()
	  <<abort(FatalError);

	Info<<"min / max / avg = "<< min(p).value() << " / " << max(p).value() << " / " << average(p).value() << endl;  
	p.write();
    }

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
