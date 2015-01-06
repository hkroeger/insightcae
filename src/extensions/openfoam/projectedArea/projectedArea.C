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
#include "wordReList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    timeSelector::addOptions();

    argList::validArgs.append("patches");
    argList::validArgs.append("direction");
    /*
    argList::validOptions.insert("p0", "reference pressure");
    argList::validOptions.insert("noShiftY", "");
    argList::validOptions.insert("writeVTK", "");
    */

#   include "setRootCase.H"
#   include "createTime.H"
    
    instantList timeDirs = timeSelector::select0(runTime, args);
    
#   include "createMesh.H"
    
    labelHashSet patches
    (
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
      )
    );
    vector dir(IStringStream(args.additionalArgs()[1])());

    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();

	scalar A=0.0;
	forAllConstIter(labelHashSet, patches, iter)
	{
	    label pI = iter.key();
	    
	    scalarField ctrb = mesh.boundary()[pI].Sf()&dir;
	    scalar Ap = gSum(pos(ctrb)*ctrb);
	    Info << " Projected area of patch "<<iter()<<" = "<<Ap<<endl;
	    A += Ap;
	}

	Info << "Projected area at time "<<runTime.timeName()<<" = "<< A << endl;  
    }

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
