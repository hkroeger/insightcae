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
    T sm = pTraits<T>(is);

    Info << "Reading field "<<ioo.name()<<"\n" << endl;
    GeometricField<T, fvPatchField, volMesh> f(ioo, mesh);

    f+=dimensioned<T>("", f.dimensions(), sm);

    Info << "Writing field "<<ioo.name()<<"\n" << endl;
    f.write();
}



template<class T>
void enforceLowerBound(const fvMesh& mesh, const IOobject& ioo, const string& valueSource)
{
    IStringStream is(valueSource);
    T sm = pTraits<T>(is);

    Info << "Reading field "<<ioo.name()<<"\n" << endl;
    GeometricField<T, fvPatchField, volMesh> f(ioo, mesh);

    f.max(dimensioned<T>("", f.dimensions(), sm));

    Info << "Writing field "<<ioo.name()<<"\n" << endl;
    f.write();
}



template<class T>
void enforceUpperBound(const fvMesh& mesh, const IOobject& ioo, const string& valueSource)
{
    IStringStream is(valueSource);
    T sm = pTraits<T>(is);

    Info << "Reading field "<<ioo.name()<<"\n" << endl;
    GeometricField<T, fvPatchField, volMesh> f(ioo, mesh);

    f.min(dimensioned<T>("", f.dimensions(), sm));

    Info << "Writing field "<<ioo.name()<<"\n" << endl;
    f.write();
}


template<class T>
void process(const Foam::argList& args, const fvMesh& mesh, const IOobject& ioo)
{
    if (UNIOF_OPTIONFOUND(args, "scale"))
        scale<T>(mesh, ioo, readScalar(IStringStream( args.options()["scale"] )()) );
    if (UNIOF_OPTIONFOUND(args, "add"))
        add<T>(mesh, ioo, args.options()["add"]);
    if (UNIOF_OPTIONFOUND(args, "enforceLowerBound"))
        enforceLowerBound<T>(mesh, ioo, args.options()["enforceLowerBound"]);
    if (UNIOF_OPTIONFOUND(args, "enforceUpperBound"))
        enforceUpperBound<T>(mesh, ioo, args.options()["enforceUpperBound"]);
}




int main(int argc, char *argv[])
{
  argList::validArgs.append("field name");

  argList::validOptions.insert("scale", "scale factor");
  argList::validOptions.insert("add", "add specified constant");
  argList::validOptions.insert("enforceLowerBound", "lower bound value");
  argList::validOptions.insert("enforceUpperBound", "upper bound value");

# include "setRootCase.H"
# include "createTime.H"
# include "createMesh.H"

  word fieldName(IStringStream( UNIOF_ADDARG(args, 0) )());

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
      process<vector>(args, mesh, ioo );
  }
  else if (UNIOF_HEADEROK(ioo, volSymmTensorField))
  {
      process<symmTensor>(args, mesh, ioo );
  }
  else if (UNIOF_HEADEROK(ioo, volTensorField))
  {
      process<tensor>(args, mesh, ioo );
  }
  else if (UNIOF_HEADEROK(ioo, volScalarField))
  {
      process<scalar>(args, mesh, ioo );
  }


  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
