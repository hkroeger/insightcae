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
#include "inflowGeneratorBaseFvPatchVectorField.H"
#include "wallDist.H"
#include "interpolationTable.H"

#include <boost/concept_check.hpp>
#include <boost/assign.hpp>

#include "pipe.h"
#include "channel.h"
#include "refdata.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace insight;


int main(int argc, char *argv[])
{
#ifdef OF16ext
  argList::validOptions.insert("writeInterval", "number of time steps between output");
#else
  argList::addOption("writeInterval", "number of time steps between output");
#endif
  
#   include "setRootCase.H"
#   include "createTime.H"
  
  label writeInterval=1000;
  if (args.optionFound("writeInterval"))
  {
    writeInterval=readLabel(IStringStream(args.option("writeInterval"))());
  }
  
#   include "createMesh.H"
  

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

  forAll(U.boundaryField(), patchI)
  {
    if (isA<inflowGeneratorBaseFvPatchVectorField>(U.boundaryField()[patchI]))
    {
      inflowGeneratorBaseFvPatchVectorField &ifpf =
	refCast<inflowGeneratorBaseFvPatchVectorField>(U.boundaryField()[patchI]);
	
      ifpf.computeConditioningFactor(writeInterval);
    }
  }

  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
