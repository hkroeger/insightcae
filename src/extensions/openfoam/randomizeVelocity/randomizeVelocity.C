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
#include "fixedGradientFvPatchFields.H"
#include "addToRunTimeSelectionTable.H"
#include "Random.H"
#include "simpleFilter.H"

#include <boost/concept_check.hpp>


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

int main(int argc, char *argv[])
{
  
   argList::validArgs.append("RMS");
  
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"
  
   scalar RMS=readScalar(IStringStream(args.additionalArgs()[0])());

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

  Random rg(1);
  volVectorField fluctuations=0.0*U;
  forAll(fluctuations, ci)
    fluctuations[ci]=(rg.vector01()-0.5*vector::one)*RMS;
  
  simpleFilter filter(mesh);
  for (int i=0; i<100; i++)
    fluctuations=filter(fluctuations);
  
  U+=fluctuations;
  U.write();

  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
