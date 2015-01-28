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


template<class T>
class profileSampler
{
  scalarField cumWeights_;
  Field<T> profile_;
  
  point p0_;
  vector axis_;
  scalar x0_, x1_;
  label n_;
  
public:
  profileSampler
  ( 
    const point& p0,
    const vector& axis, 
    scalar x0, scalar x1, label n
  )
  : cumWeights_(n-1, 0.0),
    profile_(n-1, pTraits<T>::zero),
    p0_(p0), axis_(axis),
    x0_(x0), x1_(x1), n_(n)
  {
  }

  void cumulateSample
  (
    const Field<T>& f,
    const pointField& loc,
    const scalarField& weights
  )
  {
    forAll(f, j)
    {
      scalar x = (loc[j]-p0_)&axis_;
      
      int ib=floor(double(n_-1)*(x-x0_)/(x1_-x0_)); // bin index
      
//       ib=max(0, min(n_-1, ib));
      if ( (ib>=0) && (ib<=n_-2) )
      {
	profile_[ib]+=weights[ib]*f[j];
	cumWeights_[ib]+=weights[ib];
      }
    }
  }
  
  tmp<Field<T> > operator()() const
  {
    return tmp<Field<T> >(profile_/cumWeights_);
  }
  
  bool validBin(label i) const
  {
    return mag(cumWeights_[i])>SMALL;
  }
  
  void write(Ostream& f) const
  {
    tmp<Field<T> > prof=(*this)();
    
    for (int i=0; i<n_-1; i++)
    {
      if (validBin(i))
      {
	f()
	<< ( x0_ + (x1_-x0_)*(double(i+1)/double(n_)) )
	;
	for (int j=0; j<pTraits<T>::nComponents; j++)
	  f()<< token::SPACE << component(prof()[i], j);
	f() << nl;
      }
    }
  }

};




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
	  outputDir = obr_.time().path()/".."/"postProcessing"/"binningProfile"/startTimeName;
      }
      else
      {
	  outputDir = obr_.time().path()/"postProcessing"/"binningProfile"/startTimeName;
      }

      // Create directory if does not exist.
      mkDir(outputDir);

      // Open new file at start up
      return autoPtr<OFstream>(new OFstream(outputDir/(name_+".dat")));
  }
  
  return autoPtr<OFstream>();
}

template<class T>
void extractProfiles
(
  const fvMesh& mesh,
  IOobject& fieldHeader, 
  const point &p0, 
  const vector& axis, 
  int n,
  bool sampleWalls, 
  bool sampleInterior, 
  const labelHashSet& samplePatches
)
{
  GeometricField<T, fvPatchField, volMesh> field(fieldHeader, mesh);
  
  if (sampleWalls)
  {
    scalar x0=GREAT, x1=-GREAT;
    
    forAll(mesh.boundary(), patchI)
      if (isA<wallFvPatch>(mesh.boundary()[patchI]))
      {
	x0=min(x0, min(mesh.boundaryMesh()[patchI].localPoints()&axis));
	x1=max(x1, max(mesh.boundaryMesh()[patchI].localPoints()&axis));
      }
      Info<<x0<<" "<<x1<<endl;
    profileSampler<T> ps(p0, axis, x0, x1, n);
    
    forAll(mesh.boundary(), patchI)
      if (isA<wallFvPatch>(mesh.boundary()[patchI]))
      {
	ps.cumulateSample(field.boundaryField()[patchI], mesh.boundary()[patchI].Cf(), mesh.boundary()[patchI].magSf());
      }
      
    {
      autoPtr<OFstream> f(makeFile(mesh, "walls_"+fieldHeader.name()));
      ps.write(f());
    }
  }

  if (sampleInterior)
  {
    scalar 
      x0=min(mesh.points()&axis), 
      x1=max(mesh.points()&axis);
          
    profileSampler<T> ps(p0, axis, x0, x1, n);
    
    ps.cumulateSample(field.internalField(), mesh.C().internalField(), mesh.V());
      
    {
      autoPtr<OFstream> f(makeFile(mesh, "interior_"+fieldHeader.name()));
      ps.write(f());
    }
  }

  if (samplePatches.size()>0)
  {
    scalar x0=GREAT, x1=-GREAT;
    
    word name="patches";
    forAllConstIter(labelHashSet, samplePatches, iter)
    {
      label patchI = iter.key();
      name += "_"+mesh.boundary()[patchI].name();
      x0=min(x0, min(mesh.boundaryMesh()[patchI].localPoints()&axis));
      x1=max(x1, max(mesh.boundaryMesh()[patchI].localPoints()&axis));
    }
      
    profileSampler<T> ps(p0, axis, x0, x1, n);
    
    forAllConstIter(labelHashSet, samplePatches, iter)
    {
      label patchI = iter.key();
      ps.cumulateSample(field.boundaryField()[patchI], mesh.boundary()[patchI].Cf(), mesh.boundary()[patchI].magSf());
    }
      
    {
      autoPtr<OFstream> f(makeFile(mesh, name+"_"+fieldHeader.name()));
      ps.write(f());
    }
  }

}

int main(int argc, char *argv[])
{
    timeSelector::addOptions();
    argList::validArgs.append("direction");
    argList::validArgs.append("field names");
    argList::validOptions.insert("p0", "start point of axis");
    argList::validOptions.insert("n", "number of sampling intervals");

    argList::validOptions.insert("walls", "");
    argList::validOptions.insert("patches", "patch list");
    argList::validOptions.insert("interior", "");
    
#   include "setRootCase.H"
#   include "createTime.H"
    
    point p0(pTraits<point>::zero);
    if (args.optionFound("p0"))
      p0=point(IStringStream(args.options()["p0"])());
    
    vector axis(IStringStream(args.additionalArgs()[0])());
    axis/=mag(axis);
    
    wordList fieldNames(IStringStream(args.additionalArgs()[1])());
        
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
    
    bool sampleWalls = args.optionFound("walls");
    
    bool sampleInterior = args.optionFound("interior");

    instantList timeDirs = timeSelector::select0(runTime, args);
    
#   include "createMesh.H"
    
    labelHashSet samplePatches;
    if (args.optionFound("patches"))
    {
      samplePatches =
	mesh.boundaryMesh().patchSet
	(
#ifdef OF16ext
	  wordList
#else
	  wordReList
#endif
	  (
	    IStringStream(args.options()["patches"])()
	  )
	);
    }
    
    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;
        mesh.readUpdate();
	
	forAll(fieldNames, fli)
	{
	  word fieldName=fieldNames[fli];
	  IOobject fieldHeader
	  (
	      fieldName,
	      runTime.timeName(),
	      mesh,
	      IOobject::MUST_READ,
	      IOobject::NO_WRITE
	  );
	  
	  if (fieldHeader.headerOk())
	  {
	    if (fieldHeader.headerClassName()=="volScalarField")
	      extractProfiles<scalar>(mesh, fieldHeader, p0, axis, n, sampleWalls, sampleInterior, samplePatches);
	    else if (fieldHeader.headerClassName()=="volVectorField")
	      extractProfiles<vector>(mesh, fieldHeader, p0, axis, n, sampleWalls, sampleInterior, samplePatches);
	    else
	      FatalErrorIn("main")
	       << "Unhandled field "<<fieldHeader.name()<<" of type "<<fieldHeader.headerClassName()<<endl<<abort(FatalError);
	  }
	  else
	  {
	    FatalErrorIn("main")
	      << "Field not found: "<<fieldHeader.name()<<" at time "<<runTime.timeName()<<endl<<abort(FatalError);
	  }
	  
	}
// 	scalar x0=mesh.bounds().min() & axis;
// 	scalar x1=mesh.bounds().max() & axis;
// 
// 	
// 
//         if (vfmeanheader.headerOk())
// 	{
// 	  volVectorField vf(vfmeanheader, mesh);
// 	  
// 	  writeProfile(makeFile(runTime, "viscousForceMean"), x0, x1, n, sampleProfile(axis, x0, x1, n, vf)());
// // 	  Info<<sampleProfile(axis, x0, x1, n, vf);
// 	}

    }

    Info<< "End\n" << endl;

    return 0;

}

// ************************************************************************* //
