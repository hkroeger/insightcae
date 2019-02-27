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
#include "fixedGradientFvPatchFields.H"
#include "SRFModel.H"

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    argList::validArgs.append("inletPatches");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

//     scalar D=readScalar(IStringStream(args.additionalArgs()[0])());
    labelHashSet inletPatches(
          mesh.boundaryMesh().patchSet(
            wordReList(IStringStream( UNIOF_ADDARG(args, 0) )())
           ));

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

    auto srf = SRF::SRFModel::New(U);

    // set internal field
    UNIOF_INTERNALFIELD_NONCONST( U ) = srf->velocity(mesh.C());

    forAllConstIter(labelHashSet, inletPatches, i)
    {
      label patchI=i.key();
        //if (U.boundaryField()[patchI].fixesValue())
        {
          Info << "set values on "<<mesh.boundaryMesh()[patchI].name()<<endl;
          UNIOF_BOUNDARY_NONCONST(U)[patchI] == srf->velocity(mesh.boundary()[patchI].Cf());
        }
    }



    //p.write();
    U.write();

    Info<< "End\n" << endl;
    return 0;
}

// ************************************************************************* //
