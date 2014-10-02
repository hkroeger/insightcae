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

#include "anisotropicVorton.H"
#include "transformField.H"
#include "gsl/gsl_multimin.h"

namespace Foam
{
  
anisotropicVorton::StructureParameters::StructureParameters()
{
}

anisotropicVorton::StructureParameters::StructureParameters(const dictionary&)
{
}

void anisotropicVorton::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void anisotropicVorton::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

void anisotropicVorton::StructureParameters::write(Ostream&) const
{
}

/*
scalar anisotropicVorton::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}*/

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

anisotropicVorton::anisotropicVorton()
: turbulentStructure(),
  epsilon_(0.0),
  rx_(1.0),
  ry_(1.0),
  rz_(1.0),
  sx_(1.0),
  sy_(1.0),
  sz_(1.0),
  k0_(1.0),
  C1_(1.0)
{}

anisotropicVorton::anisotropicVorton
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(readScalar(s)),
  rx_(readScalar(s)),
  ry_(readScalar(s)),
  rz_(readScalar(s)),
  sx_(readScalar(s)),
  sy_(readScalar(s)),
  sz_(readScalar(s)),
  k0_(readScalar(s)),
  C1_(readScalar(s))
{}

double anisovf(const gsl_vector *v, void *params)
{
  scalar 
    rx=gsl_vector_get(v, 0), 
    ry=gsl_vector_get(v, 1), 
    rz=gsl_vector_get(v, 2), 
    sx=gsl_vector_get(v, 3), 
    sy=gsl_vector_get(v, 4), 
    sz=gsl_vector_get(v, 5),
    C1=max(SMALL, gsl_vector_get(v, 6)),
    k0=max(SMALL, gsl_vector_get(v, 7));
    
  if (mag(sx)<SMALL) sx=SMALL;
  if (mag(sy)<SMALL) sy=SMALL;
  if (mag(sz)<SMALL) sz=SMALL;
    
  double *par = static_cast<double*>(params);
  scalar R11=par[0], R22=par[1], R33=par[2];
  scalar Lx=par[3], Ly=par[4], Lz=par[5];
  
  vector R = 23.*pow(k0,7)*pow(M_PI,7./2.)/(192.*sqrt(2.)*C1) *
  vector
  ( 
    sx*sqr(sqr(sy)*ry-sqr(sz)*rz)/sy/sz,
    sy*sqr(sqr(sx)*rx-sqr(sz)*rz)/sx/sz,
    sz*sqr(sqr(sx)*rx-sqr(sy)*ry)/sx/sy
  );
  
  vector L = (16.*sqrt(2.*M_PI)/(23.*k0)) * vector(sx, sy, sz);
  
  return 
      sqr(R[0]-R11) 	 + sqr(R[1]-R22) 	+ sqr(R[2]-R33) 
    + sqr(L[0]-Lx) 	 + sqr(L[1]-Ly) 	+ sqr(L[2]-Lz);
}


anisotropicVorton::anisotropicVorton
(
  BoostRandomGen& r, 
  const vector& loc,
  const vector& initialDelta, 
  const vector& v, 
  const symmTensor& L, 
  scalar minL,
  label creaface,
  const symmTensor& R
)
: turbulentStructure(r, loc, initialDelta, v, L, minL, creaface, R),
  epsilon_(0.0),
  rx_(1.0),
  ry_(1.0),
  rz_(1.0),
  sx_(1.0),
  sy_(1.0),
  sz_(1.0),
  k0_(1.0),
  C1_(1.0)
{
  double par[6] = {Rp_[0], Rp_[1], Rp_[2], mag(L1_), mag(L2_), mag(L3_)};

  const gsl_multimin_fminimizer_type *T = 
    gsl_multimin_fminimizer_nmsimplex2;
  gsl_multimin_fminimizer *s = NULL;
  gsl_vector *ss, *x;
  gsl_multimin_function minex_func;

  int iter = 0;
  int status;
  double size;
  int np=8;
  
  /* Starting point */
  x = gsl_vector_alloc (np);
  gsl_vector_set_all (x, 1.0);

  /* Set initial step sizes to 1 */
  ss = gsl_vector_alloc (np);
  gsl_vector_set_all (ss, 1.0);

  /* Initialize method and iterate */
  minex_func.n = np;
  minex_func.f = anisovf;
  minex_func.params = par;

  s = gsl_multimin_fminimizer_alloc (T, np);
  gsl_multimin_fminimizer_set (s, &minex_func, x, ss);

  do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate(s);
      
      if (status) 
        break;

      size = gsl_multimin_fminimizer_size (s);
      status = gsl_multimin_test_size (size, 1e-2);

//       if (status == GSL_SUCCESS)
//         {
//           printf ("converged to minimum at\n");
//         }
// 
//       printf ("%5d %10.3e %10.3e %10.3e f() = %7.3f size = %.3f\n", 
//               iter,
//               gsl_vector_get (s->x, 0), 
//               gsl_vector_get (s->x, 1), 
//               gsl_vector_get (s->x, 2), 
//               s->fval, size);
    }
  while (status == GSL_CONTINUE && iter < 1000);
  
  rx_=gsl_vector_get(s->x, 0);
  ry_=gsl_vector_get(s->x, 1);
  rz_=gsl_vector_get(s->x, 2);
  sx_=gsl_vector_get(s->x, 3);
  sy_=gsl_vector_get(s->x, 4);
  sz_=gsl_vector_get(s->x, 5);
  C1_=gsl_vector_get(s->x, 6);
  k0_=gsl_vector_get(s->x, 7);

  C1_=std::max(SMALL, C1_);
  k0_=std::max(SMALL, k0_);

  if (mag(sx_)<SMALL) sx_=SMALL;
  if (mag(sy_)<SMALL) sy_=SMALL;
  if (mag(sz_)<SMALL) sz_=SMALL;
  
//   Info<<"@"<<Rp_<<": \t"<<rx_<<" "<<ry_<<" "<<rz_<<" / \t"<<sx_<<" "<<sy_<<" "<<sz_<<" / \t"<<k0_<<" "<<C1_<<" \t #"<<iter<<endl;
  
  gsl_vector_free(x);
  gsl_vector_free(ss);
  gsl_multimin_fminimizer_free (s);  
}

anisotropicVorton::anisotropicVorton(const anisotropicVorton& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_),
  rx_(o.rx_),
  ry_(o.ry_),
  rz_(o.rz_),
  sx_(o.sx_),
  sy_(o.sy_),
  sz_(o.sz_),
  k0_(o.k0_),
  C1_(o.C1_)
{ 
}

vector anisotropicVorton::fluctuation(const StructureParameters& pa, const vector& x) const
{
    vector delta_x = x - location();
//     scalar Lx=mag(L1_), Ly=mag(L2_), Lz=mag(L3_);
    
    double Xx=delta_x&er1_;
    double Yy=delta_x&er2_;
    double Zz=delta_x&er3_;
    
//     if 
//         (
//             (mag(Xx)  < 2.*mag(L1_)) &&
//             (mag(Yy)  < 2.*mag(L2_)) &&
//             (mag(Zz)  < 2.*mag(L3_))
//         )
    {

      scalar f=
	  sqrt(1./C1_) 
	* exp( -0.25*sqr(k0_)*( sqr(Xx/sx_) + sqr(Yy/sy_) + sqr(Zz/sz_)) ) 
	* pow(k0_,7) 
	* M_PI 
	* ( sqr(k0_*sy_*sz_*Xx) + sqr(sx_)*( sqr(k0_*sz_*Yy) + sqr(sy_)*( -10.*sqr(sz_)+sqr(k0_*Zz) ) ) );
	
      vector u
      (
	-(sqr(sy_)*ry_ - sqr(sz_)*rz_)*Yy*Zz / (16.*sqr(sx_)   * pow(sy_,4) * pow(sz_,4)),
	 (sqr(sx_)*rx_ - sqr(sz_)*rz_)*Xx*Zz / (16.*pow(sx_,4) * sqr(sy_)   * pow(sz_,4)),
	-(sqr(sx_)*rx_ - sqr(sy_)*ry_)*Xx*Yy / (16.*pow(sx_,4) * pow(sy_,4) * sqr(sz_)  )
      );

      vector ut=transform( tensor(er1_, er2_, er3_).T(), f*epsilon_*u);
//       Info<<Rp_<<": "<<u<<" "<<ut<<endl;
      return ut;
    }
//   else
//     return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<anisotropicVorton> anisotropicVorton::New(Istream& s)
{
    return autoPtr<anisotropicVorton>(new anisotropicVorton(s));
}


void anisotropicVorton::randomize(BoostRandomGen& rand)
{
  //epsilon_ = 2.0*(rand.vector01() - 0.5*vector::one);
  epsilon_ = 2.0*(rand() - 0.5);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

anisotropicVorton::~anisotropicVorton()
{}


autoPtr<anisotropicVorton> anisotropicVorton::clone() const
{
    return autoPtr<anisotropicVorton>
        (
            new anisotropicVorton(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void anisotropicVorton::operator=(const anisotropicVorton& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("anisotropicVorton::operator=(const anisotropicVorton&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    turbulentStructure::operator=(rhs);
    epsilon_=rhs.epsilon_;
    rx_=rhs.rx_;
    ry_=rhs.ry_;
    rz_=rhs.rz_;
    sx_=rhs.sx_;
    sy_=rhs.sy_;
    sz_=rhs.sz_;
    k0_=rhs.k0_;
    C1_=rhs.C1_;
}

bool anisotropicVorton::operator!=(const anisotropicVorton& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_)
	||
        (rx_!=o.rx_)
	||
        (ry_!=o.ry_)
	||
        (rz_!=o.rz_)
	||
        (sx_!=o.sx_)
	||
        (sy_!=o.sy_)
	||
        (sz_!=o.sz_)
	||
        (k0_!=o.k0_)
	||
        (C1_!=o.C1_);
}

Ostream& operator<<(Ostream& s, const anisotropicVorton& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    s<<ht.rx_<<endl;
    s<<ht.ry_<<endl;
    s<<ht.rz_<<endl;
    s<<ht.sx_<<endl;
    s<<ht.sy_<<endl;
    s<<ht.sz_<<endl;
    s<<ht.k0_<<endl;
    s<<ht.C1_<<endl;
    return s;
}

Istream& operator>>(Istream& s, anisotropicVorton& ht)
{
    s >> *static_cast<turbulentStructure*>(&ht);
    ht.epsilon_=readScalar(s);
    ht.rx_=readScalar(s);
    ht.ry_=readScalar(s);
    ht.rz_=readScalar(s);
    ht.sx_=readScalar(s);
    ht.sy_=readScalar(s);
    ht.sz_=readScalar(s);
    ht.k0_=readScalar(s);
    ht.C1_=readScalar(s);
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
