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

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
    argList::validArgs.append("patches");
    argList::validOptions.insert("pName", "p");
    argList::validOptions.insert("nIter", "10");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    word pName("p");
    label niter=10;
    if (args.options().found("pName"))
    {
      pName=UNIOF_OPTION(args, "pName");
    }
    if (args.options().found("nIter"))
    {
      niter=readLabel(IStringStream(UNIOF_OPTION(args, "nIter"))());
    }

    labelHashSet patches(
          mesh.boundaryMesh().patchSet(
            wordReList(IStringStream( UNIOF_ADDARG(args, 0) )())
           ));

    Info << "Reading field "<<pName<<"\n" << endl;
    volScalarField p
    (
        IOobject
        (
            pName,
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    );

    volScalarField p_smooth
    (
        IOobject
        (
            pName+"_smooth",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        p.dimensions(),
        calculatedFvPatchField<scalar>::typeName
    );

    p_smooth=p;


    forAllConstIter(labelHashSet, patches, i)
    {
      label patchI=i.key();
      const fvPatch& patch = mesh.boundary()[patchI];

      Info << "smoothing values on "<<patch.patch().name()<<endl;

      for (label it=0; it<niter; it++)
      {
          fvPatchScalarField& pf = UNIOF_BOUNDARY_NONCONST(p_smooth)[patchI];

          //vectorField f ( pf*patch.nf() );
          scalarField Ai( patch.magSf() );
          forAll(pf, j)
          {
              Ai[j] /= scalar(patch.patch().faceFaces()[j].size());
          }
          scalarField At( patch.magSf() );
          scalarField p_smoothed( pf*patch.magSf() );

          forAll(pf, j)
          {
              const auto& ff=patch.patch().faceFaces();
              forAll(ff[j], k)
              {
                  label ni=ff[j][k];
                  p_smoothed[j] += pf[ni]*Ai[ni];
                  At[j] += Ai[ni];
              }
          }

          p_smoothed /= At;

          pf = p_smoothed;

          Info<<"#"<<it<<": force="<<gSum(pf*patch.Sf())<<endl;
      }
    }

    p_smooth.write();

    Info<< "End\n" << endl;
    return 0;
}

// ************************************************************************* //
