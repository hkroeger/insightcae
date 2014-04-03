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
#include "wallDist.H"
#include <boost/concept_check.hpp>


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

using namespace Foam;

class LengthScaleModel
{
protected:
  scalar c0_, c1_, c2_, c3_;
public:
  LengthScaleModel()
  : c0_(0.841), c1_(-0.6338), c2_(0.6217), c3_(0.778)
  {
  }
  
  LengthScaleModel(const dictionary& d)
  : c0_(readScalar(d.lookup("c0"))),
    c1_(readScalar(d.lookup("c1"))),
    c2_(readScalar(d.lookup("c2"))),
    c3_(readScalar(d.lookup("c3")))
  {
  }
  
  scalar operator()(scalar y) const
  {
    return c0_*::pow(y, c2_) + c1_*::pow(y, c3_);
  }
};

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
  LengthScaleModel L_long_;
  LengthScaleModel L_lat_;
  
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
  }

  pipeFlow(Istream& is)
  : inflowInitializer(is),
    Ubulk_(readScalar(dict_.lookup("Ubulk"))),
    D_(readScalar(dict_.lookup("D"))),
    p0_(dict_.lookupOrDefault<point>("p0", point(0,0,0))),
    axis_(dict_.lookupOrDefault<vector>("axis", vector(1,0,0))),
    L_long_(dict_.subDict("L_long")),
    L_lat_(dict_.subDict("L_lat"))
  {
  }
    
  virtual void initialize(volVectorField& U) const
  {
    inflowGeneratorBaseFvPatchVectorField& ifpf = inflowGeneratorPatchField(U);
    const fvPatch& patch=ifpf.patch();
    forAll(patch.Cf(), fi)
    {
      const point& p=patch.Cf()[fi];
      
      vector rv=p-p0_; rv-=axis_*(rv&axis_);
      scalar r=mag(rv);
      scalar y=(1.-r/(0.5*D_));
      
      ifpf.Umean()[fi]=Ubulk_ * axis_*Foam::pow(y, 1./7.);
      
      vector e_radial(rv/mag(rv));
      vector e_tan=axis_^e_radial;
      
      tensor ev(axis_, e_radial, e_tan); // eigenvectors => rows
      scalar delta=0.5*D_;
      
      tensor L = ev.T() & (delta*diagTensor(L_long_(y), L_lat_(y), L_lat_(y))) & ev;

      ifpf.L()[fi] = symmTensor(L.xx(), L.xy(), L.xz(),
					L.yy(), L.yz(),
						L.zz());
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
  
  wallDist y(mesh);
  
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
