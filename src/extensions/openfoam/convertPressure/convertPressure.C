/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "fvCFD.H"
#include "OFstream.H"
#include "transformField.H"
#include "transformGeometricField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    timeSelector::addOptions();

    argList::validArgs.append("p0");
    argList::validArgs.append("rho");
    argList::validOptions.insert("pclip", "real pressure for clipping");
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
    
    dimensionedScalar pclip("pclip", dimPressure, 0.0);
    if (args.optionFound("pclip"))
      pclip.value()=readScalar(IStringStream(args.options()["pclip"])());
    
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
	  p=max(pclip, p);
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
