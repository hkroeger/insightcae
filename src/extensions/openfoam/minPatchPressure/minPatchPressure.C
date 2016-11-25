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

    argList::validArgs.append("patches");
    argList::validArgs.append("minAFrac");
    /*
    argList::validOptions.insert("p0", "reference pressure");
    argList::validOptions.insert("noShiftY", "");
    argList::validOptions.insert("writeVTK", "");
    */

#   include "setRootCase.H"
#   include "createTime.H"
    
    instantList timeDirs = timeSelector::select0(runTime, args);
    
#   include "createMesh.H"

/*    labelHashSet patches =
      mesh.boundaryMesh().patchSet
      (
#ifdef OF16ext
	wordList
#else
	wordReList
#endif
	(
	  IStringStream(args.additionalArgs()[0])()
	)
      );*/

    word patch
	(
	  IStringStream(
#if (defined(OFplus)||defined(OFdev))
	    args.arg(1)
#else
	    args.additionalArgs()[0]
#endif
	  )()
	);
	
    label pI=mesh.boundaryMesh().findPatchID(patch);
    if (pI<0)
    {
      FatalErrorIn("main")
      <<"patch not found: "<<patch<<endl<<abort(FatalError);
    }
      
    scalar Afrac=readScalar(IStringStream(
#if (defined(OFplus)||defined(OFdev))
	    args.arg(2)
#else
	    args.additionalArgs()[1]
#endif
    )());

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
	
	const fvPatch& patch=mesh.boundary()[pI];
	
	scalarField pb=p.boundaryField()[pI];

	labelList order(patch.size());
	sortedOrder(pb, order);
	
        scalar Athreshold=Afrac*sum(patch.magSf());

	scalar A=0.0;
	scalar globalMinP=GREAT;
	forAll(order, oi)
	  {
	    label fI=order[oi];

	    A+=patch.magSf()[fI];
	    if (A>Athreshold)
	    {
	      globalMinP = pb[fI];
	      break;
	    }	      
	  }
	Info<<"Minimum pressure at t="<<runTime.timeName()<<" pmin="<<globalMinP<<endl;
    }

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
