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

Class
    gaussianSpot

Description

Author

\*----------------------------------------------------------------------------*/

#include "gaussianSpot.H"

namespace Foam
{
  
gaussianSpot::StructureParameters::StructureParameters()
{
}

gaussianSpot::StructureParameters::StructureParameters(const dictionary&)
{
}

void gaussianSpot::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void gaussianSpot::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

void gaussianSpot::StructureParameters::write(Ostream&) const
{
}

/*
scalar gaussianSpot::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}*/

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

gaussianSpot::gaussianSpot()
: turbulentStructure(),
  epsilon_(pTraits<vector>::zero)
{}

gaussianSpot::gaussianSpot
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(s)
{}

gaussianSpot::gaussianSpot(BoostRandomGen& r, const vector& loc, const vector& v, const symmTensor& L, scalar minL)
: turbulentStructure(r, loc, v, L, minL),
  epsilon_(pTraits<vector>::zero)
{
}


gaussianSpot::gaussianSpot(const gaussianSpot& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_)
{}

vector gaussianSpot::fluctuation(const StructureParameters& pa, const vector& x) const
{
    vector delta_x = x - location();

    scalar l1=mag(L1_), l2=mag(L2_), l3=mag(L3_);
    vector e1=L1_/l1, e2=L2_/l2, e3=L3_/l3;
    if 
        (
            (mag(delta_x&e1)  < l1) &&
            (mag(delta_x&e2)  < l2) &&
            (mag(delta_x&e3)  < l3)
        )
    {
      vector f=
           exp(- 4.0*magSqr(delta_x&e1)  / sqr(l1) )
          *exp(- 4.0*magSqr(delta_x&e2)  / sqr(l2) )
          *exp(- 4.0*magSqr(delta_x&e3)  / sqr(l3) )
          * pTraits<vector>::one;
	  
// #warning rotation depending on sign!
//       if ((delta_x&e1)<0.0) f[0]*=-1.0;
//       if ((delta_x&e2)<0.0) f[1]*=-1.0;
//       if ((delta_x&e3)<0.0) f[2]*=-1.0;

      return cmptMultiply(epsilon_, f) / sqrt( 0.0820292 );
    }
  else
    return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<gaussianSpot> gaussianSpot::New(Istream& s)
{
    return autoPtr<gaussianSpot>(new gaussianSpot(s));
}


void gaussianSpot::randomize(BoostRandomGen& rand)
{
  //epsilon_ = 2.0*(rand.vector01() - 0.5*vector::one);
  epsilon_.x() = 2.0*(rand() - 0.5);
  epsilon_.y() = 2.0*(rand() - 0.5);
  epsilon_.z() = 2.0*(rand() - 0.5);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

gaussianSpot::~gaussianSpot()
{}


autoPtr<gaussianSpot> gaussianSpot::clone() const
{
    return autoPtr<gaussianSpot>
        (
            new gaussianSpot(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void gaussianSpot::operator=(const gaussianSpot& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("gaussianSpot::operator=(const gaussianSpot&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    turbulentStructure::operator=(rhs);
    epsilon_=rhs.epsilon_;
}

bool gaussianSpot::operator!=(const gaussianSpot& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const gaussianSpot& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, gaussianSpot& ht)
{
    s >> *static_cast<turbulentStructure*>(&ht);
    vector eps(s);
    ht.epsilon_=eps;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
