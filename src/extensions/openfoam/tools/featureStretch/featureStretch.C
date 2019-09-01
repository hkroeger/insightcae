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

#include "edgeMesh.H"
#include "argList.H"
#include "OFstream.H"
#include "IFstream.H"
#include "boundBox.H"
#include "transformField.H"
#include "Pair.H"
#include "quaternion.H"
#include "openfoam/stretchtransformation.h"

#include "uniof.h"

using namespace Foam;
using namespace insight;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Main program:

int main(int argc, char *argv[])
{

    argList::noParallel();
    argList::validArgs.clear();

    argList::validArgs.append("feature file");
    argList::validArgs.append("output feature file");
    argList::validArgs.append("compression factor");
    argList::validArgs.append("height of compressed region");
    argList::validArgs.append("height of transistion end");
    argList::validOptions.insert("zw", "location of waterline");
    argList args(argc, argv);

    scalar compression(readScalar(IStringStream(UNIOF_ADDARG(args,2))()));
    scalar x1(readScalar(IStringStream(UNIOF_ADDARG(args,3))()));
    scalar x2(readScalar(IStringStream(UNIOF_ADDARG(args,4))()));

    scalar zw=0.0;
    if (args.options().found("zw"))
    {
      zw = readScalar(args.optionLookup("zw")());
    }

    fileName edgeFileName(UNIOF_ADDARG(args,0));

    Info<< "Reading eMesh from " << edgeFileName << " ..." << endl;

    IFstream fi(edgeFileName);
    token headerKeyWord(fi);
    dictionary header(fi);
#if OF_VERSION<010700 //def OF16ext
    edgeMesh edge1(fi); 
#else
    edgeMesh edge1; fi >> edge1; //(edgeFileName);
#endif

    stretchTransformation trans(compression, x1, x2, zw);
    
    pointField pnew(edge1.points().size());
    forAll(pnew, pi)
    {
      const point &p=edge1.points()[pi];
      pnew[pi]=toVec<Foam::vector>( trans.toBox(insight::vector(p)) ); 
    }
    
    edgeMesh edge2( pnew, edge1.edges());

    fileName outFileName(UNIOF_ADDARG(args,1));

    Info<< "Writing edge to " << outFileName << " ..." << endl;
    
    { 
      OFstream f(outFileName);
      f<<headerKeyWord<<header;
      f<<edge2;
    }
    
    Info << "End\n" << endl;

    return 0;
}


// ************************************************************************* //
