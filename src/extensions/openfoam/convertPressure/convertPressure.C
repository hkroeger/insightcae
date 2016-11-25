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

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;


int main(int argc, char *argv[])
{
//     timeSelector::addOptions();

    argList::validArgs.append("from_name");
    argList::validArgs.append("from_dim");
    argList::validArgs.append("to_name");
    argList::validArgs.append("p0");
    argList::validArgs.append("rho");
    argList::validOptions.insert("pclip", "real pressure for clipping");

#   include "setRootCase.H"
#   include "createTime.H"
    
    instantList timeDirs = timeSelector::select0(runTime, args);
    
#   include "createMesh.H"
    
    word from_name(
#if defined(OFdev)
      args[1]
#else
      args.additionalArgs()[0]
#endif
    );
    word to_name(
#if defined(OFdev)
      args[3]
#else
      args.additionalArgs()[2]
#endif
    );

    IStringStream from_dim_args(
#if defined(OFdev)
      (args[2])
#else
      args.additionalArgs()[1]
#endif
    );
    dimensionSet from_dim(from_dim_args);

    dimensionedScalar p0("p0", dimPressure, 1e5);
    p0.value()=readScalar(IStringStream(
#if defined(OFdev)
      args[4]
#else
      args.additionalArgs()[3]
#endif
    )());
    
    dimensionedScalar rho("rho", dimDensity, 1);
    rho.value()=readScalar(IStringStream(
#if defined(OFdev)
      args[5]
#else
      args.additionalArgs()[4]
#endif
    )());
    
    dimensionedScalar pclip("pclip", dimPressure, -GREAT);
    if (args.optionFound("pclip"))
      pclip.value()=readScalar(IStringStream(args.options()["pclip"])());
    
    Info<< "Time = " << runTime.timeName() << endl;

    Info << "Reading field "<<from_name<<"\n" << endl;
    autoPtr<volScalarField> from_p
    (
      new volScalarField
      (
	  IOobject
	  (
	      from_name,
	      runTime.timeName(),
	      mesh,
	      IOobject::MUST_READ,
	      IOobject::AUTO_WRITE
	  ),
	  mesh
      )
    );
    
    from_p->dimensions().reset(from_dim);
    
    autoPtr<volScalarField> to_p
    (
      new volScalarField
      (
	  IOobject
	  (
	      to_name,
	      runTime.timeName(),
	      mesh,
	      IOobject::MUST_READ,
	      IOobject::AUTO_WRITE
	  ),
	  mesh
      )
    );


    if ( from_dim == dimensionSet(1, -1, -2, 0, 0) )
    {
      // need to be converted into normalized pressure
      Info<<"Converting values into normalized pressure values."<<endl;
      to_p()=from_p()/rho;
      to_p()-=p0/rho;
    }
    else if ( from_dim == dimensionSet(0, 2, -2, 0, 0) )
    {
      Info<<"Converting values into real pressure values."<<endl;
      // need to be converted into real pressure
      /*
      to_p()=from_p()*rho;
      to_p()+=p0;
      to_p()=max(pclip, to_p());
*/
       to_p()=max(pclip, from_p()*rho+p0);

      forAll(to_p().boundaryField(), pI)
      {
	  to_p()
#ifdef OFdev
	  .boundaryFieldRef()
#else
	  .boundaryField()
#endif
	  [pI]=max(to_p().boundaryField()[pI], pclip.value());
      }
    }
    else
      FatalErrorIn("main")
      << "Don't know how the handle these pressure dimensions: "<<from_dim
      <<abort(FatalError);

    Info<<"min / max / avg = "<< min(to_p()).value() << " / " << max(to_p()).value() << " / " << average(to_p()).value() << endl;  
    to_p->write();

    Info<< "End\n" << endl;

    return 0;
}

// ************************************************************************* //
