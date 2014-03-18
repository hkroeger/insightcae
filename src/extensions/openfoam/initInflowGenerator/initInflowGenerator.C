/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Application
    yPlusRAS

Description
    Calculates and reports yPlus for all wall patches, for the specified times.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "OFstream.H"
#include "transformField.H"
#include "transformGeometricField.H"
#include "fixedGradientFvPatchFields.H"
#include "addToRunTimeSelectionTable.H"
#include "inflowGeneratorBaseFvPatchVectorField.H"
#include <boost/concept_check.hpp>


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

class inflowInitializer
{
protected:
  dictionary dict_;
  word patchName_;
  
public:
  TypeName("inflowInitializer");
  
  declareRunTimeSelectionTable
        (
            autoPtr,
            inflowInitializer,
            istream,
            (
                Istream& is
            ),
            (is)
        );
	
  //- Selector
  static autoPtr<inflowInitializer > New
  (
      Istream& is
  )
  {
    word typeName(is);
    Info<< "Selecting initializer type " << typeName << endl;

    istreamConstructorTable::iterator cstrIter =
        istreamConstructorTablePtr_->find(typeName);

    if (cstrIter == istreamConstructorTablePtr_->end())
    {
        FatalErrorIn
        (
            "inflowInitializer::New()"
        )   << "Unknown turbulenceModel type " << typeName
            << endl << endl
            << "Valid inflowInitializer types are :" << endl
            << istreamConstructorTablePtr_->toc()
            << exit(FatalError);
    }

    return autoPtr<inflowInitializer>(cstrIter()(is));
  }

  inflowInitializer
  (
      const word& patchName
  )
  : patchName_(patchName)
  {}

  inflowInitializer(Istream& is)
  : dict_(is),
    patchName_(dict_.lookup("patchName"))
  {}
  
  virtual ~inflowInitializer() 
  {}
  
  inflowGeneratorBaseFvPatchVectorField& inflowGeneratorPatchField(volVectorField& U) const
  {
    label patchI=U.mesh().boundaryMesh().findPatchID(patchName_);
    if (patchI<0)
    {
      FatalErrorIn("initialize")
      << "Patch "<<patchName_<<" does not exist!"
      << abort(FatalError);
    }
    
    return refCast<inflowGeneratorBaseFvPatchVectorField>(U.boundaryField()[patchI]);
  }
  
  virtual void initialize(volVectorField& U) const =0;
  
  virtual autoPtr<inflowInitializer> clone() const =0;
};

defineTypeNameAndDebug(inflowInitializer, 0);
defineRunTimeSelectionTable(inflowInitializer, istream);



class pipeFlow
: public inflowInitializer
{
  scalar Ubulk_;
  scalar D_;
  point p0_;
  vector axis_;
  
public:
  TypeName("pipeFlow");
  
  pipeFlow
  (
    const word& patchName,
    scalar Ubulk,
    scalar D,
    point p0 = point(0, 0, 0),
    vector axis = vector(1, 0, 0)
  )
  : inflowInitializer(patchName),
    Ubulk_(Ubulk),
    D_(D), p0_(p0), axis_(axis)
  {
    Info<<Ubulk_<<endl;
  }

  pipeFlow(Istream& is)
  : inflowInitializer(is),
    Ubulk_(readScalar(dict_.lookup("Ubulk"))),
    D_(readScalar(dict_.lookup("D"))),
    p0_(dict_.lookupOrDefault<point>("p0", point(0,0,0))),
    axis_(dict_.lookupOrDefault<vector>("axis", vector(1,0,0)))
  {
    Info<<Ubulk_<<endl;
  }
    
  virtual void initialize(volVectorField& U) const
  {
    scalar L=0.1*D_;
    
    inflowGeneratorBaseFvPatchVectorField& ifpf = inflowGeneratorPatchField(U);
    const fvPatch& patch=ifpf.patch();
    forAll(patch.Cf(), fi)
    {
      const point& p=patch.Cf()[fi];
      vector rv=p-p0_; rv-=axis_*(rv&axis_);
      scalar r=mag(rv);
      
      ifpf.Umean()[fi]=Ubulk_ * axis_*Foam::pow(2.*(1.-r/D_), 1./7.);
      ifpf.L()[fi]=symmTensor(1, 0, 0, 1, 0, 1) * max(0.05, 2.*(1.-r/D_))*L;
    }
  }

  virtual autoPtr<inflowInitializer> clone() const
  {
    return autoPtr<inflowInitializer>(new pipeFlow(patchName_, Ubulk_, D_, p0_, axis_));
  }

};

defineTypeNameAndDebug(pipeFlow, 0);
addToRunTimeSelectionTable(inflowInitializer, pipeFlow, istream);
    
int main(int argc, char *argv[])
{
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"
  
  Info<< "Reading combustion properties\n" << endl;

  IOdictionary inflowProperties
  (
      IOobject
      (
	  "inflowProperties",
	  runTime.constant(),
	  mesh,
	  IOobject::MUST_READ,
	  IOobject::NO_WRITE
      )
  );

  PtrList<inflowInitializer> inits(inflowProperties.lookup("initializers"));

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

  forAll(inits, i)
    inits[i].initialize(U);
  
  U.write();

  Info<< "End\n" << endl;

  return 0;
}

// ************************************************************************* //
