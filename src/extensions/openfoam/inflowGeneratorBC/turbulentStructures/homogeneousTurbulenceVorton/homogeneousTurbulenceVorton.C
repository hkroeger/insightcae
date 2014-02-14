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
    homogeneousTurbulence

Description

Author

\*----------------------------------------------------------------------------*/

#include "homogeneousTurbulence.H"

namespace Foam
{

homogeneousTurbulence::Parameters::Parameters
(
)
    :
    L_(0.0),    // integral length scale
    eta_(0.0),  // Kolmogorov length
    Cl_(0.0),
    Ceta_(0.0)
{
    C_2=0.0;
    C_3=0.0;    
}

homogeneousTurbulence::Parameters::Parameters
(
    const dictionary& dict
)
    :
    L_(readScalar(dict.lookup("L"))),    // integral length scale
    eta_(readScalar(dict.lookup("eta"))),  // Kolmogorov length
    Cl_(readScalar(dict.lookup("Cl"))),
    Ceta_(readScalar(dict.lookup("Ceta")))
{
    C_2=5.087*pow(Ceta_,(-1.1));
    C_3=0.369*Cl_;    
}

homogeneousTurbulence::Parameters::Parameters
(
    scalar L,    // integral length scale
    scalar eta,  // Kolmogorov length
    scalar Cl,
    scalar Ceta

):
    L_(L),    // integral length scale
    eta_(eta),  // Kolmogorov length
    Cl_(Cl),
    Ceta_(Ceta)
{
    C_2=5.087*pow(Ceta_,(-1.1));
    C_3=0.369*Cl_;    
}

void homogeneousTurbulence::Parameters::write
(
    Ostream& os
) const
{
    os.writeKeyword("L")
        << L_ << token::END_STATEMENT << nl;
    os.writeKeyword("eta")
        << eta_ << token::END_STATEMENT << nl;
    os.writeKeyword("Cl")
        << Cl_ << token::END_STATEMENT << nl;
    os.writeKeyword("Ceta")
        << Ceta_ << token::END_STATEMENT << nl;
}

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

scalar homogeneousTurbulence::calcInfluenceLength(const Parameters& p)
{
    scalar dyy=p.eta_/10.0;
    scalar yyy=1e-8;
    scalar funct=1.0;
    scalar amax=0.0;
    scalar criterion=1.0;

    while(criterion>1e-3)
    {
        scalar reta =yyy/p.eta_;
        scalar aleta=yyy/p.L_;

        funct=
            pow(
                reta/ pow( pow(reta,2.5)+p.C_2, 0.4),
                2.166667
                )
                /(pow(yyy,1.166667))
                /(p.C_3*pow(aleta, 1.83333)+1.000);

        if (funct>amax) amax=funct;
        if (yyy>10*p.eta_) criterion=funct/amax;
        yyy=yyy+dyy;
    }   

    return yyy;
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

homogeneousTurbulence::homogeneousTurbulence()
: 
    location_(pTraits<vector>::zero),
    omegav_(pTraits<vector>::zero)
{
}

homogeneousTurbulence::homogeneousTurbulence
(
    Istream& s
)
: 
    location_(s),
    omegav_(s)
{
    Info<<location_<<endl;
}

homogeneousTurbulence::homogeneousTurbulence(const vector& loc)
:
    location_(loc),
    omegav_(pTraits<vector>::zero)
{}


homogeneousTurbulence::homogeneousTurbulence(const homogeneousTurbulence& o)
:
    location_(o.location_),
    omegav_(o.omegav_)
{}

vector homogeneousTurbulence::fluctuation(const Parameters& p, const vector& x) const
{

    vector delta_x = x - location_;

    scalar radiika=magSqr(delta_x);

    vector t=delta_x ^ omegav_;

    scalar tmod=mag(t);

    if (tmod>SMALL) t/=tmod;
    tmod=max(1e-5, tmod);

    scalar sradiika=sqrt(radiika);
    scalar reta =sradiika/p.eta_;
    scalar aleta=sradiika/p.L_;

    scalar sperva=
        (
            pow(reta/pow(pow(reta,2.5)+p.C_2, 0.4), 2.166667)
        )
        /pow(sradiika,1.166667)
        /(p.C_3*pow(aleta, 1.83333)+1.000)
        *tmod/sradiika;
        
    return sperva*t;
        
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<homogeneousTurbulence> homogeneousTurbulence::New(Istream& s)
{
    Info<<"reading"<<endl;
    return autoPtr<homogeneousTurbulence>(new homogeneousTurbulence(s));
}


void homogeneousTurbulence::randomize(Random& rand)
{
    omegav_ = rand.vector01() - 0.5*pTraits<vector>::one;
    omegav_/=mag(omegav_);
    for (label i=0;i<3;i++) omegav_[i]=min(1.0,max(-1.0, omegav_[i]));
}

void homogeneousTurbulence::moveForward(vector delta)
{
    location_+=delta;
}

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

homogeneousTurbulence::~homogeneousTurbulence()
{}


autoPtr<homogeneousTurbulence> homogeneousTurbulence::clone() const
{
    return autoPtr<homogeneousTurbulence>
        (
            new homogeneousTurbulence(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void homogeneousTurbulence::operator=(const homogeneousTurbulence& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("homogeneousTurbulence::operator=(const homogeneousTurbulence&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    location_=rhs.location_;
    omegav_=rhs.omegav_;
}

bool homogeneousTurbulence::operator!=(const homogeneousTurbulence& o) const
{
    return 
        (location_!=o.location_)
        ||
        (omegav_!=o.omegav_);
}

Ostream& operator<<(Ostream& s, const homogeneousTurbulence& ht)
{
    s<<ht.location_<<endl;
    s<<ht.omegav_<<endl;
    return s;
}

Istream& operator>>(Istream& s, homogeneousTurbulence& ht)
{
    vector loc(s);
    vector om(s);
    ht.location_=loc;
    ht.omegav_=om;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
