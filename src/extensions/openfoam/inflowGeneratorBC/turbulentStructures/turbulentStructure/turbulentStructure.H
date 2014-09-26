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

#ifndef turbulentStructure_H
#define turbulentStructure_H

#include "fvCFD.H"
#include "boostRandomGen.H"
#include "IOdictionary.H"
#include "Istream.H"
#include "Ostream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#include <boost/graph/graph_concepts.hpp>

namespace Foam
{

class inflowGeneratorBaseFvPatchVectorField;
class turbulentStructure;

Ostream& operator<<(Ostream&, const turbulentStructure&);
Istream& operator>>(Istream&, turbulentStructure&);


class ESAnalyze
{
  tensor es_;
public:
  ESAnalyze(const symmTensor& t);
  bool clip(scalar minL);
  inline vector c1() const { return es_.x(); }
  inline vector c2() const { return es_.y(); }
  inline vector c3() const { return es_.z(); }
  inline tensor Leig() const { return es_; }
  scalar Lalong(const vector& x) const;

  static scalar Lalong(const vector& x, const vector& L1, const vector& L2, const vector& L3);  
  static tensor eigenSystem(const symmTensor& L);
};

/**
 * base class for turbulent structures.
 * derived from point => represents its location
 */
class turbulentStructure
: public point
{
protected:
  /**
   * convection velocity of the structure
   */
  vector velocity_;
  
  /**
   * prescribed turbulent length scale, converted into (principal axes)*(length of ellipsoids semiaxis)
   */
  vector L1_, L2_, L3_;
  
  point startPoint_; 

  /**
   * intersection point of motion path and (global) patch
   */
  point footPoint_;
  
  /**
   * label of the face on which the spot was created
   */
  label creaFace_;
  
  /**
   * principal components of prescribed Reynolds Stresses
   */
  vector Rp_;
  
  /**
   * principal directions of prescribed Reynolds Stresses
   */
  vector er1_, er2_, er3_;
  
public:    

    /**
     * default
     */
    turbulentStructure();
    
    /**
     * Create from stream (during restart)
     */
    turbulentStructure(Istream&);
    
    /**
     * create at some location (during run)
     */
    turbulentStructure
    (
      BoostRandomGen&, 
      const point& p, 
      const vector& initialDelta, 
      const vector& v, 
      const symmTensor& L, scalar minL,
      label creaface,
      const symmTensor& R
    );

    /**
     * copy constructor
     */
    turbulentStructure(const turbulentStructure& p);
    
    inline void setVelocity(const vector& x) { velocity_=x; }
    inline void setLocation(const point& x) { point::operator=(x); }
    inline const point& footPoint() const { return footPoint_; }
    
    inline void initialPositioning(const point& x, const vector& initialDelta) 
    { 
      startPoint_=x+initialDelta; 
      point::operator=(startPoint_); 
      footPoint_=x; 
    }
    
    inline const point& location() const { return *this; }
    inline const vector& velocity() const { return velocity_; }
    inline const vector& L(label i) const 
    { 
      switch (i)
      {
	case 0: return L1_; break;
	case 1: return L2_; break;	
	case 2: return L3_; break;      
	default: 
	  FatalErrorIn("L()") << "Prinicipal length scale "<<i<<" is undefined"<<abort(FatalError); 
	  return vector::zero;
      }
    }
    
    inline scalar Lmax() const
    {
      return Foam::max(mag(L1_), Foam::max(mag(L2_), mag(L3_)));
    }
    
    
    scalar travelledDistance() const;
    scalar passedThrough() const;
    inline label creaFace() const { return creaFace_; }
    inline void setCreaFace(label cf) { creaFace_=cf; }
    
    // Edit
    void moveForward(scalar dt);

    void operator=(const turbulentStructure&);
    
    friend Ostream& operator<<(Ostream&, const turbulentStructure&);
    friend Istream& operator>>(Istream&, turbulentStructure&);
    
    static tensor Lund(const symmTensor& R);
    
    inline symmTensor R() const 
    { 
      tensor t(er1_, er2_, er3_);
      return symm( t.T() & diagTensor(Rp_[0], Rp_[1], Rp_[2]) & t ); 
    }
    
    inline vector LundScaled(const vector& uPrime) const
    {
      return Lund( R() ) & uPrime;
    }
    
    inline scalar Lalong(const vector& x) const
    {
      return ESAnalyze::Lalong(x, L1_, L2_, L3_);
    }
    
    inline scalar vol() const
    {
      return mag(L1_)*mag(L2_)*mag(L3_);
    }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
