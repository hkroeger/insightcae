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
#include "IFstream.H"
#include "boundBox.H"
#include "transformField.H"
#include "Pair.H"
#include "quaternion.H"
#include "openfoam/stretchtransformation.h"
#include "vector.H"

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Main program:

using namespace Foam;

int main(int argc, char *argv[])
{
  argList::validArgs.append("compression factor");
  argList::validArgs.append("height of compressed region");
  argList::validArgs.append("height of transistion end");
  argList::validOptions.insert("zw", "location of waterline");

# include "setRootCase.H"
# include "createTime.H"
# include "createMesh.H"

  scalar compression(readScalar(IStringStream(UNIOF_ADDARG(args,0))()));
  scalar x1(readScalar(IStringStream(UNIOF_ADDARG(args,1))()));
  scalar x2(readScalar(IStringStream(UNIOF_ADDARG(args,2))()));

  scalar zw=0.0;
  if (args.options().found("zw"))
    {
      zw = readScalar(args.optionLookup("zw")());
    }

  insight::stretchTransformation trans(compression, x1, x2, zw);
  
  Info << "Writing mesh." << endl;
  pointField pnew(mesh.points().size());
  forAll(pnew, pi)
  {
    const point &p=mesh.points()[pi];
    pnew[pi]=insight::toVec<Foam::vector>( trans.toWedge(insight::vector(p)) );
  }
  
  mesh.movePoints( pnew );
  mesh.checkMesh(true);
  
  //runTime++;
  mesh.write();

  Info << "End\n" << endl;

  return 0;
}


// ************************************************************************* //
