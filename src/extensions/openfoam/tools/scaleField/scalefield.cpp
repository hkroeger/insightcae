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

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //




using namespace Foam;




template<class T>
void scale(const fvMesh& mesh, const IOobject& ioo, scalar s)
{
    if (fabs(s-1.0)>SMALL)
    {
        Info << "Reading field "<<ioo.name()<<"\n" << endl;
        GeometricField<T, fvPatchField, volMesh> f(ioo, mesh);

        f*=s;

        Info << "Writing field "<<ioo.name()<<"\n" << endl;
        f.write();
    }
}


template<class T>
void add(const fvMesh& mesh, const IOobject& ioo, const string& valueSource)
{
    IStringStream is(valueSource);
    T sm(is);

    Info << "Reading field "<<ioo.name()<<"\n" << endl;
    GeometricField<T, fvPatchField, volMesh> f(ioo, mesh);

    f+=dimensioned<T>("", f.dimensions(), sm);

    Info << "Writing field "<<ioo.name()<<"\n" << endl;
    f.write();
}

template<>
void add<scalar>(const fvMesh& mesh, const IOobject& ioo, const string& valueSource)
{
    IStringStream is(valueSource);
    scalar sm=readScalar(is);

    Info << "Reading field "<<ioo.name()<<"\n" << endl;
    GeometricField<scalar, fvPatchField, volMesh> f(ioo, mesh);

    f+=dimensioned<scalar>("", f.dimensions(), sm);

    Info << "Writing field "<<ioo.name()<<"\n" << endl;
    f.write();
}




int main(int argc, char *argv[])
{
  argList::validArgs.append("field name");
  argList::validArgs.append("scale factor");

  argList::validOptions.insert("add", "add specified constant");

# include "setRootCase.H"
# include "createTime.H"
# include "createMesh.H"

  word fieldName(IStringStream( UNIOF_ADDARG(args, 0) )());

  scalar s = readScalar(IStringStream( UNIOF_ADDARG(args, 1) )());

  IOobject ioo
  (
      fieldName,
      runTime.timeName(),
      mesh,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
  );


  if (UNIOF_HEADEROK(ioo, volVectorField))
  {
      scale<vector>(mesh, ioo, s);
      if (UNIOF_OPTIONFOUND(args, "add"))
          add<vector>(mesh, ioo, args.options()["add"]);
  }
  else if (UNIOF_HEADEROK(ioo, volSymmTensorField))
  {
      scale<symmTensor>(mesh, ioo, s);
      if (UNIOF_OPTIONFOUND(args, "add"))
          add<symmTensor>(mesh, ioo, args.options()["add"]);
  }
  else if (UNIOF_HEADEROK(ioo, volTensorField))
  {
      scale<tensor>(mesh, ioo, s);
      if (UNIOF_OPTIONFOUND(args, "add"))
          add<tensor>(mesh, ioo, args.options()["add"]);
  }
  else if (UNIOF_HEADEROK(ioo, volScalarField))
  {
      scale<scalar>(mesh, ioo, s);
      if (UNIOF_OPTIONFOUND(args, "add"))
          add<scalar>(mesh, ioo, args.options()["add"]);
  }


  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
