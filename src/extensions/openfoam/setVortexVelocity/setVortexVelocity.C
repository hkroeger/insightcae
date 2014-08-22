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
#include "fixedGradientFvPatchFields.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    argList::validArgs.append("Gamma");
    argList::validArgs.append("vc");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    scalar Gamma=readScalar(IStringStream(args.additionalArgs()[0])());
    point center(IStringStream(args.additionalArgs()[1])());
    
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

    Info << "Reading field U\n" << endl;
    volVectorField U
    (
	IOobject
	(
	    "U",
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::AUTO_WRITE
	),
	mesh
    );
    
    vector axis(0,0,1);

    PtrList<fvPatchScalarField> newpPatchFields(mesh.boundary().size());

    forAll(newpPatchFields, patchI)
    {
	if (U.boundaryField()[patchI].fixesValue())
	{
	    newpPatchFields.set
	    (
		patchI,
		new fixedGradientFvPatchScalarField
		(
		    mesh.boundary()[patchI],
		    p.dimensionedInternalField()
		)
	    );
	    newpPatchFields[patchI] == p.boundaryField()[patchI];
	}
	else
	{
	    newpPatchFields.set
	    (
		patchI,
		p.boundaryField()[patchI].clone()
	    );
	}
    }

    tmp<volScalarField> pNew
    (
	new volScalarField
	(
	    IOobject
	    (
		"p",
		mesh.time().timeName(),
		mesh,
		IOobject::NO_READ,
		IOobject::NO_WRITE,
		false
	    ),
	    mesh,
	    p.dimensions(),
	    p.internalField(),
	    newpPatchFields
	)
    );

    forAll(mesh.boundary(), patchI)
    {
      const fvPatch& patch = mesh.boundary()[patchI];
      fvPatchVectorField& Up = U.boundaryField()[patchI];
      fvPatchScalarField& pp = pNew().boundaryField()[patchI];
      if (Up.fixesValue())
      {
	vectorField r=(patch.Cf() - center);
	r-=axis*(r&axis);
	vectorField er = r/(mag(r)+SMALL);
	
	vectorField etan=axis ^ er;
	Up == etan * Gamma / (2.0*M_PI*mag(r));
      
	fixedGradientFvPatchScalarField& ppfg = static_cast<fixedGradientFvPatchScalarField&>(pp);
	ppfg.gradient() = -patch.nf() & (er*( sqr(Gamma / 2. / M_PI) / pow(mag(r),3)) );
      }
    }
    

    //p.write();
    U.write();
    pNew().write();

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
