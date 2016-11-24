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
#include "wallFvPatch.H"

#include "Tuple2.H"
#include "token.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

tmp<scalarField> sampleProfile
(
 const vector& axis, 
 scalar x0, scalar x1, label n, 
 const volVectorField& f
)
{
  const fvMesh& mesh = f.mesh();
  
  scalarField nsamples(n-1, 0.0);
  scalarField profile(n-1, 0.0);
  
  forAll(mesh.boundary(), patchI)
    if (isA<wallFvPatch>(mesh.boundary()[patchI]))
    {
      const fvPatchVectorField& pf=f.boundaryField()[patchI];
      forAll(pf, j)
      {
	scalar x=mesh.Cf().boundaryField()[patchI][j]&axis;
	int ib=floor(double(n-1)*(x-x0)/(x1-x0));
	//int i=max(0, min(n-2, ib));
	if ( (ib>=0) && (ib<=n-2) )
	{
	  profile[ib]+=pf[j]&axis;
	  nsamples[ib]+=1.0;
	}
      }
    }
    
  return tmp<scalarField>(profile/max(1.0,nsamples));
}

void writeProfile
(
 autoPtr<OFstream> f,
 scalar x0, scalar x1, label n, 
 const scalarField& prof
)
{
  if (f.valid())
  {
    for (int i=0; i<n-1; i++)
    {
      f()
      << ( x0 + (x1-x0)*(double(i+1)/double(n)) )
      << token::SPACE
      << prof[i]
      << nl;
    }
  }
}

autoPtr<OFstream> makeFile(const objectRegistry& obr_, const word& name_)
{
  // File update
  if (Pstream::master())
  {
      fileName outputDir;
      word startTimeName =
	  obr_.time().timeName(obr_.time().value());

      if (Pstream::parRun())
      {
	  // Put in undecomposed case (Note: gives problems for
	  // distributed data running)
	  outputDir = obr_.time().path()/".."/"postProcessing"/"viscousForceProfile"/startTimeName;
      }
      else
      {
	  outputDir = obr_.time().path()/"postProcessing"/"viscousForceProfile"/startTimeName;
      }

      // Create directory if does not exist.
      mkDir(outputDir);

      // Open new file at start up
      return autoPtr<OFstream>(new OFstream(outputDir/(name_+".dat")));
  }
  
  return autoPtr<OFstream>();
}

int main(int argc, char *argv[])
{
    timeSelector::addOptions();
    argList::validArgs.append("direction");
    argList::validOptions.insert("n", "number of sampling intervals");
    
#   include "setRootCase.H"
#   include "createTime.H"
    
    vector axis(IStringStream(args.additionalArgs()[0])());
    axis/=mag(axis);
        
    label n=2;
    if (args.optionFound("n"))
    {
      n=readLabel(IStringStream(args.options()["n"])())+1;
      if (n<2)
      {
	FatalErrorIn("viscousForceProfile::main")
	<<"At least 1 sampling interval is required, specified: "
	<<(n-1)
	<<abort(FatalError);
      }
    }
    instantList timeDirs = timeSelector::select0(runTime, args);
    
#   include "createMesh.H"
    
    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();
	
	scalar x0=mesh.bounds().min() & axis;
	scalar x1=mesh.bounds().max() & axis;

	IOobject vfheader
	(
	    "viscousForce",
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::NO_WRITE
	);
	IOobject vfmeanheader
	(
	    "viscousForceMean",
	    runTime.timeName(),
	    mesh,
	    IOobject::MUST_READ,
	    IOobject::NO_WRITE
	);
	
#if defined(OFplus)
        if (vfheader.typeHeaderOk<volVectorField>())
#else
        if (vfheader.headerOk())
#endif
	{
	  volVectorField vf(vfheader, mesh);
	  
	  writeProfile(makeFile(runTime, "viscousForce"), x0, x1, n, sampleProfile(axis, x0, x1, n, vf)());
	}

#if defined(OFplus)
        if (vfmeanheader.typeHeaderOk<volVectorField>())
#else
        if (vfmeanheader.headerOk())
#endif
	{
	  volVectorField vf(vfmeanheader, mesh);
	  
	  writeProfile(makeFile(runTime, "viscousForceMean"), x0, x1, n, sampleProfile(axis, x0, x1, n, vf)());
// 	  Info<<sampleProfile(axis, x0, x1, n, vf);
	}

    }

    Info<< "End\n" << endl;

    return 0;

}

// ************************************************************************* //
