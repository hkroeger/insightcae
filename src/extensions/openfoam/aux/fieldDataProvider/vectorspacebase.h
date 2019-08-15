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
 */

#ifndef FOAM_VECTORSPACEBASE_H
#define FOAM_VECTORSPACEBASE_H

#include "fvCFD.H"
#include "transform.H"

namespace Foam
{

    
    
  
class VectorSpaceBase
{
protected:
  point p0_;
  vector ep_/*, ex_, ez_*/;
public:
  void read(Istream&is);
  void writeSup(Ostream& os) const;
  
  inline const point& origin() const { return p0_; }
  
  inline scalar t(const point& p) const
  {
    return (p-origin())&ep_;
  }
  
  template<class T>
  T operator()(const T& org) const
  {
//     vector ey = - (ex_ ^ ez_);
//     tensor tt(ex_, ey, ez_);    
//     
//     return transform(tt, org);
    return org;
  }

};




class CylCoordVectorSpaceBase
: public VectorSpaceBase
{
public:
  inline const vector& ax() const { return ep_; }
  
  inline scalar t(const point& p) const
  {
    vector r=(p-origin());
    r-=ax()*(r&ax());
    return mag(r);
  }
  
  template<class T>
  T operator()(const T& org, const point& p) const
  {
    vector er=p-origin();
    er-=ax()*(er&ax());
    if (mag(er)<SMALL)
    {
     er=vector(1,0,0);
     if ( (1.-mag(er&ax())) < SMALL ) 
       er=vector(0,1,0);
    }
    er/=mag(er);
    
    
    vector et = (ax() ^ er);
    tensor tt(ax(), et, er);    
    
    return transform(tt.T(), org);
  }
};




}

#endif
