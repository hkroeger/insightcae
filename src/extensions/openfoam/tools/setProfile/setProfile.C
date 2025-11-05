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

#include "Tuple2.H"
#include "interpolationTable.H"

#include "uniof.h"

#include "fielddataprovider.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //



using namespace Foam;



template<class T>
void setProfile
(
    GeometricField<T, fvPatchField, volMesh>& field,
    double t,
    const string& source
)
{
    auto fdp = FieldDataProvider<T>::New(
        IStringStream(source)());

    UNIOF_INTERNALFIELD_NONCONST(field)
        = fdp()(t, field.mesh().C());
}



int main(int argc, char *argv[])
{
    argList::validArgs.append("fieldname");    
    argList::validArgs.append("sourceExpression");
    argList::validOptions.insert("t", "time");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    word fieldname( UNIOF_ADDARG(args, 0) );
    string source( UNIOF_ADDARG(args, 1) );

    double t=runTime.value();
    if (UNIOF_OPTIONFOUND(args, "t"))
    {
        t=args.optionRead<scalar>("t");
    }
        
    IOobject header
	(
	    fieldname,
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::AUTO_WRITE
	);


    if (UNIOF_HEADEROK(header, volScalarField))
    {
        volScalarField field(header, mesh);
        setProfile<Foam::scalar>(field, t, source);
        field.write();
    }
    else 
    if (UNIOF_HEADEROK(header, volVectorField))
    {
        volVectorField field(header, mesh);
        setProfile<Foam::vector>(field, t, source);
        field.write();
    }
    else
    {
        FatalErrorIn("main")
         << "Could not find field "<<fieldname<<" of type scalar or vector!"
         <<abort(FatalError);
    }
    
    Info<<"End."<<endl;

    return 0;
}

// ************************************************************************* //
