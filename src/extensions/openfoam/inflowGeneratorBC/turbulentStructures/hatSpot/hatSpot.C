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

hatSpot::Parameters::Parameters
(
)
    :
    L_(0.0),    // integral length scale
    Lspot_(calcInfluenceLength(*this))
{
}

hatSpot::Parameters::Parameters
(
    const dictionary& dict
)
    :
    L_(readScalar(dict.lookup("L"))),    // integral length scale
    Lspot_(calcInfluenceLength(*this))
{
}

hatSpot::Parameters::Parameters
(
    scalar L    // integral length scale

):
    L_(L),    // integral length scale
    Lspot_(calcInfluenceLength(*this))
{
}

void hatSpot::Parameters::write
(
    Ostream& os
) const
{
    os.writeKeyword("L")
        << L_ << token::END_STATEMENT << nl;
}

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

scalar hatSpot::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

hatSpot::hatSpot()
: 
    location_(pTraits<vector>::zero),
    epsilon_(pTraits<vector>::zero)
{
}

hatSpot::hatSpot
(
    Istream& s
)
: 
    location_(s),
    epsilon_(s)
{
    Info<<location_<<endl;
}

hatSpot::hatSpot(const vector& loc)
:
    location_(loc),
    epsilon_(pTraits<vector>::zero)
{}


hatSpot::hatSpot(const hatSpot& o)
:
    location_(o.location_),
    epsilon_(o.epsilon_)
{}

vector hatSpot::fluctuation(const Parameters& p, const vector& x) const
{
    vector delta_x = x - location_;

    if 
        (
            (delta_x.x()  < p.Lspot_ / 2.0) &&
            (delta_x.y()  < p.Lspot_ / 2.0) &&
            (delta_x.z()  < p.Lspot_ / 2.0)
        )
    {
      vector f=
           (1.0 - 2.0*delta_x.x()  / p.Lspot_)
          *(1.0 - 2.0*delta_x.y()  / p.Lspot_)
          *(1.0 - 2.0*delta_x.z()  / p.Lspot_)
          * pTraits<vector>::one;

      return cmptMultiply(epsilon_, f);
    }
  else
    return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<hatSpot> hatSpot::New(Istream& s)
{
    Info<<"reading"<<endl;
    return autoPtr<hatSpot>(new hatSpot(s));
}


void hatSpot::randomize(Random& rand)
{
    rand.randomise(epsilon_);
    epsilon_ -= pTraits<vector>::one*0.5;
    epsilon_ *= 2.0;
}

void hatSpot::moveForward(vector delta)
{
    location_+=delta;
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

    location_=rhs.location_;
    epsilon_=rhs.epsilon_;
}

bool hatSpot::operator!=(const hatSpot& o) const
{
    return 
        (location_!=o.location_)
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const hatSpot& ht)
{
    s<<ht.location_<<endl;
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, hatSpot& ht)
{
    vector loc(s);
    vector eps(s);
    ht.location_=loc;
    ht.epsilon_=eps;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
