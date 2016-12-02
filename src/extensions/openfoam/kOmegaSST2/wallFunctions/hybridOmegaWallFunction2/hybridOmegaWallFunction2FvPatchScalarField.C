/*---------------------------------------------------------------------------*\ 
| File modified by Engys Ltd 2010                                             |
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2008-2009 OpenCFD Ltd.
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

\*---------------------------------------------------------------------------*/

#include "hybridOmegaWallFunction2FvPatchScalarField.H"
#include "RASModel.H"
#include "kOmegaSST2.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "addToRunTimeSelectionTable.H"
#include "wallFvPatch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
#if not (defined(OFplus)||defined(OFdev))
namespace incompressible
{
#endif
namespace RASModels
{


// Wandfunktion von Rung
/*
scalar fYPlus
(
 scalar kappa, scalar E, scalar nu, scalar U, scalar y, scalar Utau, int order
)
{
    scalar ret=1.0, fak=1.0;
    scalar Uplus=U/Utau;
    for (int i=1; i<=order; i++)
    {
        fak*=i;
        ret+=pow(kappa*Uplus, i)/fak;
    }
    return Uplus+(exp(kappa*Uplus)-ret)/E - y*Utau/nu;
}


scalar dfYPlus_dUtau
(
 scalar kappa, scalar E, scalar nu, scalar U, scalar y, scalar Utau, int order
)
{
    scalar ret=0.0, fak=1.0;
    for (int i=1; i<=order; i++)
    {
        fak*=i;
        ret += -scalar(i)*pow(kappa*U, i)*pow(Utau, -i-1)/fak;
    }
    return -U*pow(Utau, -2) + ( exp(kappa*U/Utau)*(-kappa*U*pow(Utau, -2)) - ret)/E - y/nu;
}
*/

// Wandfunktion von Reichardt

scalar fYPlus
(
 scalar kappa, scalar E, scalar nu, scalar U, scalar y, scalar Utau, int order
)
{
  scalar yPlus=y*Utau/nu;
  return 
    (log(1.+0.4*yPlus)/kappa)
      +7.8*
    (
     1.-exp(-yPlus/11.)
     -(yPlus/11.)*exp(-yPlus/3.)
     )
    -(U/Utau);
}

  
scalar dfYPlus_dUtau
(
 scalar kappa, scalar E, scalar nu, scalar U, scalar y, scalar Utau, int order
 )
{
    scalar yPlus=y*Utau/nu;
    return 
      (
       (0.4/(kappa*(1.+0.4*yPlus)))
       +(7.8/11.)*(exp(-yPlus/11.)-(2./3.)*exp(-yPlus/3.)))*(y/nu)
      +(U/sqr(Utau));
}


      /*
scalar Leqn
(
    scalar kappa, scalar eps, scalar k, scalar y, scalar cmu75, scalar nu
)
{
    scalar cc=kappa*y/(cmu75*pow(k, 1.5));
    scalar bb=0.2*sqr(k)/nu;
    return log(1.-1./(eps*cc))+bb/eps;
}

scalar dLeqndeps
(
    scalar kappa, scalar eps, scalar k, scalar y, scalar cmu75, scalar nu
)
{
    scalar cc=kappa*y/(cmu75*pow(k, 1.5));
    scalar bb=0.2*sqr(k)/nu;
    return (1./(1.-1./(eps*cc)))*(1./(sqr(eps)*cc))-bb/sqr(eps);
}
      */

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void hybridOmegaWallFunction2FvPatchScalarField::checkType()
{
    if (!isA<wallFvPatch>(patch()))
    {
        FatalErrorIn("hybridOmegaWallFunction2FvPatchScalarField::checkType()")
            << "Invalid wall function specification" << nl
            << "    Patch type for patch " << patch().name()
            << " must be wall" << nl
            << "    Current patch type is " << patch().type() << nl << endl
            << abort(FatalError);
    }
}


void hybridOmegaWallFunction2FvPatchScalarField::patchTypeFaceWeighting() const
{
	faceWeightsPtr_ = new Field<scalar>(this->size(), 0);
	Field<scalar>& faceWeights(*faceWeightsPtr_);

	const labelList& faceCells = this->patch().faceCells();

	const fvMesh& mesh(this->patch().boundaryMesh().mesh());
	
	label nInternalFaces = mesh.nInternalFaces();
	
	forAll(faceCells, i)
	  {
	    label cellI = faceCells[i];
	    
	    const labelList& cellFaces = mesh.cells()[cellI];
		
	    forAll(cellFaces, cfI)
	      {
		label faceI = cellFaces[cfI];
		
		if (faceI >= nInternalFaces)
		  {
		    label facePatch = mesh.boundaryMesh().whichPatch(faceI);
				
		    typedef GeometricField<scalar, fvPatchField, volMesh> gF;
				
		    const gF& f
		      (
		       mesh.objectRegistry::lookupObject<gF>
		       (this->
#ifdef OFdev
		        internalField()
#else
		        dimensionedInternalField()
#endif
		       .name())
		       );
				
		    if 
		      ( 
		       isA<fixedInternalValueFvPatchField<scalar> >
		       (f.boundaryField()[facePatch])
			)
		      {
			const fvPatch& fp = mesh.boundary()[facePatch];
			label startFace = fp.patch().start();
			
			faceWeights[i] += fp.magSf()[faceI - startFace];
		      }
		  }
	      }
	  }
	
	faceWeights = this->patch().magSf() / faceWeights;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

hybridOmegaWallFunction2FvPatchScalarField::hybridOmegaWallFunction2FvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedInternalValueFvPatchField<scalar>(p, iF),
    UName_("U"),
    kName_("k"),
    GName_("RASModel::G"),
    nuName_("nu"),
    nutName_("nut"),
    Cmu_(0.09),
    kappa_(0.41),
    E_(9.8),
    tw_(0.057),
    faceWeightsPtr_(NULL)
{
    checkType();
}


hybridOmegaWallFunction2FvPatchScalarField::hybridOmegaWallFunction2FvPatchScalarField
(
    const hybridOmegaWallFunction2FvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedInternalValueFvPatchField<scalar>(ptf, p, iF, mapper),
    UName_(ptf.UName_),
    kName_(ptf.kName_),
    GName_(ptf.GName_),
    nuName_(ptf.nuName_),
    nutName_(ptf.nutName_),
    Cmu_(ptf.Cmu_),
    kappa_(ptf.kappa_),
    E_(ptf.E_),
    tw_(ptf.tw_),
    faceWeightsPtr_(NULL)
{
    checkType();
}


hybridOmegaWallFunction2FvPatchScalarField::hybridOmegaWallFunction2FvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedInternalValueFvPatchField<scalar>(p, iF, dict),
    UName_(dict.lookupOrDefault<word>("U", "U")),
    kName_(dict.lookupOrDefault<word>("k", "k")),
    GName_(dict.lookupOrDefault<word>("G", "RASModel::G")),
    nuName_(dict.lookupOrDefault<word>("nu", "nu")),
    nutName_(dict.lookupOrDefault<word>("nut", "nut")),
    Cmu_(dict.lookupOrDefault<scalar>("Cmu", 0.09)),
    kappa_(dict.lookupOrDefault<scalar>("kappa", 0.41)),
    E_(dict.lookupOrDefault<scalar>("E", 9.8)),
    tw_(dict.lookupOrDefault<scalar>("tw", 0.057)),
    faceWeightsPtr_(NULL)
{
    checkType();
}


hybridOmegaWallFunction2FvPatchScalarField::hybridOmegaWallFunction2FvPatchScalarField
(
    const hybridOmegaWallFunction2FvPatchScalarField& owfpsf
)
:
    fixedInternalValueFvPatchField<scalar>(owfpsf),
    UName_(owfpsf.UName_),
    kName_(owfpsf.kName_),
    GName_(owfpsf.GName_),
    nuName_(owfpsf.nuName_),
    nutName_(owfpsf.nutName_),
    Cmu_(owfpsf.Cmu_),
    kappa_(owfpsf.kappa_),
    E_(owfpsf.E_),
    tw_(owfpsf.tw_),
    faceWeightsPtr_(NULL)
{
    checkType();
}


hybridOmegaWallFunction2FvPatchScalarField::hybridOmegaWallFunction2FvPatchScalarField
(
    const hybridOmegaWallFunction2FvPatchScalarField& owfpsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedInternalValueFvPatchField<scalar>(owfpsf, iF),
    UName_(owfpsf.UName_),
    kName_(owfpsf.kName_),
    GName_(owfpsf.GName_),
    nuName_(owfpsf.nuName_),
    nutName_(owfpsf.nutName_),
    Cmu_(owfpsf.Cmu_),
    kappa_(owfpsf.kappa_),
    E_(owfpsf.E_),
    tw_(owfpsf.tw_),
    faceWeightsPtr_(NULL)
{
    checkType();
}

hybridOmegaWallFunction2FvPatchScalarField::~hybridOmegaWallFunction2FvPatchScalarField()
{
  deleteDemandDrivenData(faceWeightsPtr_);
}

const Foam::Field<Foam::scalar>& 
hybridOmegaWallFunction2FvPatchScalarField::faceWeights() const
{
	if (!faceWeightsPtr_)
	{
		patchTypeFaceWeighting();
	}
	
	return *faceWeightsPtr_;
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void hybridOmegaWallFunction2FvPatchScalarField::updateCoeffs()
{
  if (!this->updated())
    {
//       const fvMesh& mesh(patch().boundaryMesh().mesh());

      const kOmegaSST2& rasModel 
	= db().lookupObject<kOmegaSST2>(
#if defined(OFplus)||defined(OFdev)
	"turbulenceProperties"
#else
	"RASProperties"
#endif
	);

      const scalar yPlusLam = rasModel.yPlusLam(kappa_, E_);
      const scalarField& yw = rasModel.y()[patch().index()];
      
//       const scalar Cmu25 = pow(Cmu_, 0.25);
//       const scalar Cmu75 = pow(Cmu_, 0.75);
      
      volScalarField& G = const_cast<volScalarField&>
	(db().lookupObject<volScalarField>(GName_));

      volScalarField& omega = const_cast<volScalarField&>
	(db().lookupObject<volScalarField>
	 (
#ifdef OFdev
	      internalField()
#else
	      dimensionedInternalField()
#endif      
	   .name()));

//       const scalarField& k = db().lookupObject<volScalarField>(kName_);

      const scalarField& nuw =
	patch().lookupPatchField<volScalarField, scalar>(nuName_);

      const scalarField& nutw =
	patch().lookupPatchField<volScalarField, scalar>(nutName_);

      const fvPatchVectorField& Uw =
	patch().lookupPatchField<volVectorField, vector>(UName_);

      const volVectorField& Uvol =
	db().lookupObject<volVectorField>(UName_);

      fvPatchScalarField& yPlusw = const_cast<fvPatchScalarField&>
	(patch().lookupPatchField<volScalarField, scalar>("yPlus"));

      fvPatchScalarField& Utauw = const_cast<fvPatchScalarField&>
	(patch().lookupPatchField<volScalarField, scalar>("Utau"));

      const scalarField magGradUw 
	= mag(Uw.snGrad() - patch().nf() * (Uw.snGrad() & patch().nf()));

      // Set omega and G
      forAll(nutw, faceI)
    	{
	  /*
	    label faceCellI = patch().faceCells()[faceI];

	    //yPlus from wall shear to allow for laminar region
	    scalar tauw = (nutw[faceI]+nuw[faceI])*magGradUw[faceI];
	    scalar utau = max(SMALL,::sqrt(tauw));

	    scalar yPlus = utau*y[faceI]/nuw[faceI];

	    //star hybrid formulation for k production and omega

	    //phi blending
	    scalar phiHL = sqr(1.0-exp(-yPlus/yPlusLam));

	    //inverse apparent cell thickness
	    scalar sv = patch().magSf()[faceI]/mesh.V()[faceCellI];

	    G[faceCellI] += 
	    faceWeights()[faceI] *
	    (
	    phiHL *
	    (
	    tauw*magGradUw[faceI]*y[faceI]*sv
	    -Cmu75 * sv/kappa_*log(E_*yPlus)
	    *::sqrt(k[faceCellI]) * k[faceCellI]
	    )
	    - (1.0-phiHL) *
	    (
	    4.0*nuw[faceI]*sqr(sv)*k[faceCellI]
	    )
	    );

	    //STAR continuous WF formulation for Omega
	    scalar Rek = y[faceI]*::sqrt(k[faceCellI])/nuw[faceI];
	    scalar Lomega = y[faceI]*(1.0-exp(-tw_*Rek));
	    omega[faceCellI] += faceWeights()[faceI] *::sqrt(k[faceCellI])
	    /(kappa_*Cmu25*Lomega);
	  */
	  label faceCelli = patch().faceCells()[faceI];
	  scalar U=mag(Uvol[faceCelli]-Uw[faceI]);
	  scalar y=yw[faceI];

	  if (debug>=2)
	    Info<<"U="<<U<<" y="<<y<<endl;

	  // Startwerte:
	  scalar yPlus = y/nuw[faceI];
	  scalar Uplus= max(SMALL, U); 

	  scalar Utau=max(SMALL, U/Uplus);

	  int iter=0;
	  scalar f=0;

	  if (debug>=2)
	    Info<<"Start: Utau="<<Utau<<" f="<<f<<endl;

	  for (; iter<100; iter++)
	    {
	      f=fYPlus
		(
		 kappa_, 
		 E_, 
		 nuw[faceI], 
		 U, 
		 y, 
		 Utau, 
		 9
		 );
	      
	      if (debug>=2)
		{
		  Info<<iter
		      <<": Utau="<<Utau
		      <<" yPlus="<<y*Utau/nuw[faceI]
		      <<" f="<<f<<endl;
		}
		    
	      if (mag(f)<1e-7) break;
	      Utau -= 
		f 
		/
		dfYPlus_dUtau
		(
		 kappa_, 
		 E_, 
		 nuw[faceI],
		 U, 
		 y, 
		 Utau, 
		 9
		 );
	      
	      Utau=max(SMALL, Utau);
	      
	    }
	  yPlus=y*Utau/nuw[faceI];

	  yPlusw[faceI]=yPlus;
	  Utauw[faceI]=Utau;

	  if (iter>=99)
	    WarningIn("hybridOmegaWallFunction2FvPatchScalarField.C")
	      <<"Maximum number of iterations reached. Final residual f="<<f
	      <<" (Utau="<<Utau<<" y+="<<yPlus<<")"
	      <<endl;

	  // For corner cells (with two boundary or more faces),
	  // omega and G in the near-wall cell are calculated
	  // as an average
	  //cellBoundaryFaceCount[faceCelli]++;
	  
	  scalar omega_vis=(6.0*nuw[faceI])
	    /(rasModel.beta1().value()*sqr(y));
	  scalar omega_log=Utau
	    /(kappa_*y*sqrt(rasModel.betaStar().value()));

	  /*
	    scalar omega_b1=omega_vis+omega_log;
	    scalar omega_b2=::pow( ::pow(omega_vis, 1.2) + ::pow(omega_log, 1.2), 1./1.2);
	    scalar phi=::tanh( ::pow(yPlus/10., 4) );
	    tau_[faceCelli ]+= 1.0/(phi*omega_b1 + (1.0-phi)*omega_b2);
	  */
	  omega[faceCelli] += sqrt(sqr(omega_vis) + sqr(omega_log));

	  if (yPlus > yPlusLam)
	    {
	      G[faceCelli] += pow(Utau, 3)/(kappa_*y);
	    }
	  else
	    {
	      //G[faceCelli]+=0.0;
	    }
    	}
    }

  fvPatchField<scalar>::updateCoeffs();
}


void hybridOmegaWallFunction2FvPatchScalarField::write(Ostream& os) const
{
    fixedInternalValueFvPatchField<scalar>::write(os);
    writeEntryIfDifferent<word>(os, "U", "U", UName_);
    writeEntryIfDifferent<word>(os, "k", "k", kName_);
    writeEntryIfDifferent<word>(os, "G", "RASModel::G", GName_);
    writeEntryIfDifferent<word>(os, "nu", "nu", nuName_);
    writeEntryIfDifferent<word>(os, "nut", "nut", nutName_);
    os.writeKeyword("Cmu") << Cmu_ << token::END_STATEMENT << nl;
    os.writeKeyword("kappa") << kappa_ << token::END_STATEMENT << nl;
    os.writeKeyword("E") << E_ << token::END_STATEMENT << nl;
    os.writeKeyword("tw") << tw_ << token::END_STATEMENT << nl;
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField
(
    fvPatchScalarField,
    hybridOmegaWallFunction2FvPatchScalarField
);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
#if not (defined(OFplus)||defined(OFdev))
} // End namespace incompressible
#endif
} // End namespace Foam

// ************************************************************************* //
