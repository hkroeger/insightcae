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

#include "homogeneousTurbulenceVorton.H"

namespace Foam
{

homogeneousTurbulenceVorton::StructureParameters::StructureParameters
(
)
: eta_(0.0),  // Kolmogorov length
  Cl_(0.0),
  Ceta_(0.0)
{
    C_2=0.0;
    C_3=0.0;    
}

homogeneousTurbulenceVorton::StructureParameters::StructureParameters
(
    const dictionary& dict
)
: eta_(readScalar(dict.lookup("eta"))),  // Kolmogorov length
  Cl_(readScalar(dict.lookup("Cl"))),
  Ceta_(readScalar(dict.lookup("Ceta")))
{
    C_2=5.087*pow(Ceta_,(-1.1));
    C_3=0.369*Cl_;    
}

void homogeneousTurbulenceVorton::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void homogeneousTurbulenceVorton::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

// homogeneousTurbulenceVorton::StructureParameters::StructureParameters
// (
//     scalar eta,  // Kolmogorov length
//     scalar Cl,
//     scalar Ceta
// 
// ):
//     eta_(eta),  // Kolmogorov length
//     Cl_(Cl),
//     Ceta_(Ceta)
// {
//     C_2=5.087*pow(Ceta_,(-1.1));
//     C_3=0.369*Cl_;    
// }
// 
void homogeneousTurbulenceVorton::StructureParameters::write
(
    Ostream& os
) const
{
    os.writeKeyword("eta") << eta_ << token::END_STATEMENT << nl;
    os.writeKeyword("Cl") << Cl_ << token::END_STATEMENT << nl;
    os.writeKeyword("Ceta") << Ceta_ << token::END_STATEMENT << nl;
}

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

scalar homogeneousTurbulenceVorton::StructureParameters::calcInfluenceLength(const vector& Lv)
{
  scalar L=mag(Lv);
  
    scalar dyy=eta_/10.0;
    scalar yyy=1e-8;
    scalar funct=1.0;
    scalar amax=0.0;
    scalar criterion=1.0;

    while(criterion>1e-3)
    {
        scalar reta =yyy/eta_;
        scalar aleta=yyy/L;

        funct=
            ::pow(
                reta/ ::pow( ::pow(reta,2.5)+C_2, 0.4),
                2.166667
                )
                /(::pow(yyy,1.166667))
                /(C_3*::pow(aleta, 1.83333)+1.000);

        if (funct>amax) amax=funct;
        if (yyy>10*eta_) criterion=funct/amax;
        yyy=yyy+dyy;
    }   

    return yyy;
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

homogeneousTurbulenceVorton::homogeneousTurbulenceVorton()
: 
    turbulentStructure(),
    omegav_(pTraits<vector>::zero)
{
}

homogeneousTurbulenceVorton::homogeneousTurbulenceVorton
(
  Istream& s
)
: 
    turbulentStructure(s),
    omegav_(s)
{
}

homogeneousTurbulenceVorton::homogeneousTurbulenceVorton(BoostRandomGen& r, const point& loc, const vector& initialDelta, const vector& v,  const tensor& Leig,
  label creaface,
      const symmTensor& R)
:
    turbulentStructure(r, loc, initialDelta, v, Leig, creaface, R),
    omegav_(pTraits<vector>::zero)
{
}


homogeneousTurbulenceVorton::homogeneousTurbulenceVorton(const homogeneousTurbulenceVorton& o)
:
    turbulentStructure(o),
    omegav_(o.omegav_)
{}

vector homogeneousTurbulenceVorton::fluctuation(const StructureParameters& pa, const vector& x) const
{
  scalar L = (mag(L1_)+mag(L2_)+mag(L3_)) / 3.0;
 /* 
  vector delta_x = x - location();

  scalar radiika=magSqr(delta_x);

  vector t=delta_x ^ omegav_;

  scalar tmod=mag(t);

  if (tmod>SMALL) t/=tmod;
  tmod=Foam::max(1e-5, tmod);

  scalar sradiika=::sqrt(radiika);
  scalar reta =sradiika/pa.eta_;
  scalar aleta=sradiika/L;

  scalar sperva=
      (
	  ::pow(reta/::pow(::pow(reta,2.5)+pa.C_2, 0.4), 2.166667)
      )
      /::pow(sradiika,1.166667)
      /(pa.C_3*::pow(aleta, 1.83333)+1.000)
      *tmod/sradiika;
      
  return sperva*t;
   */
  vector dv = x - location();
  scalar nrm2 = magSqr(dv);
  vector t = dv ^ omegav_;

  return (1./L)*exp(-(M_PI/2)*nrm2/(L*L))*t;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<homogeneousTurbulenceVorton> homogeneousTurbulenceVorton::New(Istream& s)
{
    return autoPtr<homogeneousTurbulenceVorton>(new homogeneousTurbulenceVorton(s));
}


void homogeneousTurbulenceVorton::randomize(BoostRandomGen& rand)
{
    omegav_ = 2.0* (vector(rand(), rand(), rand()) - 0.5*pTraits<vector>::one);
    omegav_/=mag(omegav_);
    for (label i=0; i<3; i++) omegav_[i]=Foam::min(1.0,Foam::max(-1.0, omegav_[i]));
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

homogeneousTurbulenceVorton::~homogeneousTurbulenceVorton()
{}


autoPtr<homogeneousTurbulenceVorton> homogeneousTurbulenceVorton::clone() const
{
    return autoPtr<homogeneousTurbulenceVorton>
        (
            new homogeneousTurbulenceVorton(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void homogeneousTurbulenceVorton::operator=(const homogeneousTurbulenceVorton& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("homogeneousTurbulenceVorton::operator=(const homogeneousTurbulenceVorton&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    turbulentStructure::operator=(rhs);
    omegav_=rhs.omegav_;
}

bool homogeneousTurbulenceVorton::operator!=(const homogeneousTurbulenceVorton& o) const
{
    return 
        (location()!=o.location())
        ||
        (omegav_!=o.omegav_);
}

Ostream& operator<<(Ostream& s, const homogeneousTurbulenceVorton& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.omegav_<<endl;
    return s;
}

Istream& operator>>(Istream& s, homogeneousTurbulenceVorton& ht)
{
    s >> *static_cast<turbulentStructure*>(&ht);
    vector om(s);
    ht.omegav_=om;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
