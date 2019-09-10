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

/**
  * This tool fixes missing values after mapping by inserting appropriate values from neighbouring cells
  * Required e.g. for pressure fields in compressible cases.
  */

#include "fvCFD.H"

#include "uniof.h"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Main program:

int main(int argc, char *argv[])
{

    argList::noParallel();
    argList::validArgs.clear();

    argList::validArgs.append("field name");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    word fieldName(IStringStream(UNIOF_ADDARG(args,0))());
    scalar threshold=SMALL;

    volScalarField p
    (
          IOobject
          (
              fieldName,
              runTime.timeName(),
              mesh,
              IOobject::MUST_READ,
              IOobject::AUTO_WRITE
          ),
          mesh
    );

    bool needMoreFix=false;

    label iter=0;
    do
    {
      label n_fixed=0, n_skipped=0;

      forAll(p, i)
      {
        if (p[i]<threshold)
        {
          // go through neighbours; set average of valid neighbours; if no valid neighbours skip cell and retry in next loop
          auto nei_i = mesh.cellCells()[i];
          scalar mean_val=0.0;
          label n_valid=0;
          forAll(nei_i, j)
          {
            label i_n = nei_i[j];
            if (p[i_n]>threshold)
            {
              n_valid++;
              mean_val+=p[i_n];
            }
          }
          if (n_valid>0)
          {
            p[i]=mean_val/scalar(n_valid);
            n_fixed++;
          }
          else
          {
            needMoreFix=true;
            n_skipped++;
          }
        }
      }

      reduce(n_fixed, sumOp<label>());
      reduce(n_skipped, sumOp<label>());

      Info<<"Iter #"<<iter<<": fixed cells = "<<n_fixed<<", skipped cells = "<<n_skipped<<endl;

      iter++;
    }
    while (needMoreFix);

    p.write();

    Info << "End\n" << endl;

    return 0;
}


// ************************************************************************* //
