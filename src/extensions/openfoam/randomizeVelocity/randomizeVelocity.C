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
#include "addToRunTimeSelectionTable.H"
#include "Random.H"
#include "simpleFilter.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

int main(int argc, char *argv[])
{
  
   argList::validArgs.append("RMS");
  
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"
   
   scalar RMS=readScalar(IStringStream(
#ifdef OFdev
    args.arg(1)
#else
    args.additionalArgs()[0]
#endif
  )());

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
  for (int i=0; i<10; i++)
    fluctuations=filter(fluctuations);
  
  U+=fluctuations;
  U.write();

  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
