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
  epsilon_(0.0)
{}

anisotropicVorton::anisotropicVorton
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(readScalar(s))
{}

double anisovf(const gsl_vector *v, void *params)
{
  scalar rx=gsl_vector_get(v, 0), ry=gsl_vector_get(v, 1), rz=gsl_vector_get(v, 2), Lx=gsl_vector_get(v, 3), Ly=gsl_vector_get(v,4), Lz=gsl_vector_get(v,5);
  double *par = static_cast<double*>(params);
  scalar R11=par[0], R22=par[1], R33=par[2];
  scalar k0=par[3];
  scalar C1=par[4];
  scalar f=23.*pow(k0,7)*pow(M_PI,7./2.)/(64.*sqrt(2.)*C1);
  vector R
  ( 
    Lx*sqr(sqr(Ly)*ry-sqr(Lz)*rz)/Ly/Lz,
    Ly*sqr(sqr(Lx)*rx-sqr(Lz)*rz)/Lx/Lz,
    Lz*sqr(sqr(Lx)*rx-sqr(Ly)*ry)/Lx/Ly
  );
  
  return sqr(R[0]-R11) + sqr(R[1]-R22) + sqr(R[2]-R33);
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
  epsilon_(0.0)
{
  double k0=1.0, C1=1.0;
  double par[5] = {Rp_[0], Rp_[1], Rp_[2], k0, C1};

  const gsl_multimin_fminimizer_type *T = 
    gsl_multimin_fminimizer_nmsimplex2;
  gsl_multimin_fminimizer *s = NULL;
  gsl_vector *ss, *x;
  gsl_multimin_function minex_func;

  size_t iter = 0;
  int status;
  double size;

  /* Starting point */
  x = gsl_vector_alloc (6);
  gsl_vector_set_all (x, 1.0);

  /* Set initial step sizes to 1 */
  ss = gsl_vector_alloc (6);
  gsl_vector_set_all (ss, 1.0);

  /* Initialize method and iterate */
  minex_func.n = 6;
  minex_func.f = anisovf;
  minex_func.params = par;

  s = gsl_multimin_fminimizer_alloc (T, 6);
  gsl_multimin_fminimizer_set (s, &minex_func, x, ss);

  do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate(s);
      
      if (status) 
        break;

      size = gsl_multimin_fminimizer_size (s);
      status = gsl_multimin_test_size (size, 1e-2);

      if (status == GSL_SUCCESS)
        {
          printf ("converged to minimum at\n");
        }

      printf ("%5d %10.3e %10.3e %10.3e f() = %7.3f size = %.3f\n", 
              iter,
              gsl_vector_get (s->x, 0), 
              gsl_vector_get (s->x, 1), 
              gsl_vector_get (s->x, 2), 
              s->fval, size);
    }
  while (status == GSL_CONTINUE && iter < 100);
  
  rx_=gsl_vector_get(x, 0);
  ry_=gsl_vector_get(x, 1);
  rz_=gsl_vector_get(x, 2);
  L1_=er1_*gsl_vector_get(x, 3);
  L2_=er2_*gsl_vector_get(x, 4);
  L3_=er3_*gsl_vector_get(x, 5);
  
  gsl_vector_free(x);
  gsl_vector_free(ss);
  gsl_multimin_fminimizer_free (s);  
}

anisotropicVorton::anisotropicVorton(const anisotropicVorton& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_),
  rx_(o.rx_),
  ry_(o.ry_),
  rz_(o.rz_)
{ 
}

vector anisotropicVorton::fluctuation(const StructureParameters& pa, const vector& x) const
{
    double k0=1.0, C1=1.0;

    vector delta_x = x - location();
    scalar l1=mag(L1_), l2=mag(L2_), l3=mag(L3_);
    vector e1=L1_/l1, e2=L2_/l2, e3=L3_/l3;
    
    double Xx=delta_x&e1;
    double Yy=delta_x&e2;
    double Zz=delta_x&e3;
    
    if 
        (
            (mag(Xx)  < (l1 / 2.0)) &&
            (mag(Yy)  < (l2 / 2.0)) &&
            (mag(Zz)  < (l3 / 2.0))
        )
    {
      scalar f=-sqrt(1./C1)*exp(-0.25*k0*k0*(sqr(Xx/l1)+sqr(Yy/l2)+sqr(Zz/l3))) * pow(k0,7) * M_PI *
        ( sqr(k0*l2*l3*Xx) +sqr(l1)*(sqr(k0*l3*Yy)+sqr(l2)*(sqr(-10.*l3)+sqr(k0*Zz))) );
	
      vector u
      (
	(sqr(l2)*ry_ - sqr(l3)*rz_)*Yy*Zz / (16.*sqr(l1)   * pow(l2,4) * pow(l3,4)),
	(sqr(l1)*rx_ - sqr(l3)*rz_)*Xx*Zz / (16.*pow(l1,4) * sqr(l2)   * pow(l3,4)),
	(sqr(l1)*rx_ - sqr(l2)*ry_)*Xx*Yy / (16.*pow(l1,4) * pow(l2,4) * sqr(l3)  )
      );

      return transform( tensor(er1_, er2_, er3_), f*epsilon_*u);
    }
  else
    return pTraits<vector>::zero;
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
}

bool anisotropicVorton::operator!=(const anisotropicVorton& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const anisotropicVorton& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, anisotropicVorton& ht)
{
    s >> *static_cast<turbulentStructure*>(&ht);
    scalar eps=readScalar(s);
    ht.epsilon_=eps;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
