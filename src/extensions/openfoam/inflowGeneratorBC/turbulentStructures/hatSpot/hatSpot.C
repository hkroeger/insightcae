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
    hatSpot

Description

Author

\*----------------------------------------------------------------------------*/

#include "hatSpot.H"

namespace Foam
{
  
hatSpot::StructureParameters::StructureParameters()
{
}

hatSpot::StructureParameters::StructureParameters(const dictionary&)
{
}

void hatSpot::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void hatSpot::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

void hatSpot::StructureParameters::write(Ostream&) const
{
}

/*
scalar hatSpot::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}*/

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

hatSpot::hatSpot()
: turbulentStructure(),
  epsilon_(pTraits<vector>::zero)
{}

hatSpot::hatSpot
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(s)
{}

hatSpot::hatSpot(const vector& loc, const vector& v, const symmTensor& L)
: turbulentStructure(loc, v, L),
  epsilon_(pTraits<vector>::zero)
{
}


hatSpot::hatSpot(const hatSpot& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_)
{}

vector hatSpot::fluctuation(const vector& x) const
{
    vector delta_x = x - location();

    scalar l1=mag(L1_), l2=mag(L2_), l3=mag(L3_);
    if 
        (
            ((delta_x&(L1_/l1))  < l1 / 2.0) &&
            ((delta_x&(L2_/l2))  < l2 / 2.0) &&
            ((delta_x&(L3_/l3))  < l3 / 2.0)
        )
    {
      vector f=
           (1.0 - 2.0*(delta_x&(L1_/l1))  / l1 )
          *(1.0 - 2.0*(delta_x&(L2_/l2))  / l2 )
          *(1.0 - 2.0*(delta_x&(L3_/l3))  / l3 )
          * pTraits<vector>::one;

      return cmptMultiply(epsilon_, f);
    }
  else
    return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<hatSpot> hatSpot::New(Istream& s)
{
    return autoPtr<hatSpot>(new hatSpot(s));
}


void hatSpot::randomize(Random& rand)
{
    rand.randomise(epsilon_);
    epsilon_ -= pTraits<vector>::one*0.5;
    epsilon_ *= 2.0;
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

hatSpot::~hatSpot()
{}


autoPtr<hatSpot> hatSpot::clone() const
{
    return autoPtr<hatSpot>
        (
            new hatSpot(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void hatSpot::operator=(const hatSpot& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("hatSpot::operator=(const hatSpot&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    point::operator=(rhs);
    epsilon_=rhs.epsilon_;
}

bool hatSpot::operator!=(const hatSpot& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const hatSpot& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, hatSpot& ht)
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
