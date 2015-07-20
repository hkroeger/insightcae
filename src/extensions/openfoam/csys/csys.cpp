/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  hannes <email>
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

#include "csys.h"
#include "addToRunTimeSelectionTable.H"
#include "base/exception.h"

namespace Foam
{
  
defineTypeNameAndDebug(csys, 0);
defineRunTimeSelectionTable(csys, dictionary);

csys::csys()
{}

csys::csys(const dictionary& dict)
{}

csys::~csys()
{}

autoPtr<csys> csys::New
(
    const dictionary& dict
)
{
    word coordType = dict.lookup("type");

    dictionaryConstructorTable::iterator cstrIter =
        dictionaryConstructorTablePtr_->find(coordType);

    if (cstrIter == dictionaryConstructorTablePtr_->end())
    {
        FatalIOErrorIn
        (
            "coordinateSystem::New(const objectRegistry&, const dictionary&)",
            dict
        )   << "Unknown coordinateSystem type "
            << coordType << nl << nl
            << "Valid coordinateSystem types are :" << nl
            << dictionaryConstructorTablePtr_->sortedToc()
            << exit(FatalIOError);
    }

    return autoPtr<csys>(cstrIter()(dict));
}


defineTypeNameAndDebug(cartesian_csys, 0);
addToRunTimeSelectionTable(csys, cartesian_csys, dictionary);

cartesian_csys::cartesian_csys()
: orig_(point::zero),
  ex_(1,0,0),
  ey_(0,1,0),
  ez_(0,0,1)
{}

cartesian_csys::cartesian_csys(const dictionary& dict)
: csys(dict),
  orig_(dict.lookup("orig")),
  ex_(dict.lookup("ex")),
  ez_(dict.lookup("ez"))
{
  if (mag(ex_)<SMALL)
    throw insight::Exception("invalid direction ex specified: vector magnitude is zero!");
  if (mag(ez_)<SMALL)
    throw insight::Exception("invalid direction ez specified: vector magnitude is zero!");
  
  ex_/=mag(ex_);
  ez_/=mag(ez_);
  ey_=ez_^ex_;
  if (mag(ey_)<SMALL)
    throw insight::Exception("invalid directions ex and ez specified: vector magnitude of resulting ey is zero!");
  ey_/=mag(ey_);
}

const point& cartesian_csys::origin() const
{
  return orig_;
}

vector cartesian_csys::localPointToGlobal(const point& p) const
{
  return orig_+p.x()*ex_+p.y()*ey_+p.z()*ez_;
}

vector cartesian_csys::localVectorToGlobal(const point&, const vector& v) const
{
  return v.x()*ex_ + v.y()*ey_ + v.z()*ez_;
}

autoPtr<csys> cartesian_csys::clone() const
{
  return autoPtr<csys>(new cartesian_csys(*this));
}


defineTypeNameAndDebug(cylindrical_csys, 0);
addToRunTimeSelectionTable(csys, cylindrical_csys, dictionary);

cylindrical_csys::cylindrical_csys()
: orig_(point::zero),
  ez_(0,0,1),
  er_(1,0,0),
  e3_(0,1,0)
{}

cylindrical_csys::cylindrical_csys(const dictionary& dict)
: csys(dict),
  orig_(dict.lookup("orig")),
  ez_(dict.lookup("ez")),
  er_(dict.lookup("er"))
{
  if (mag(er_)<SMALL)
    throw insight::Exception("invalid direction er specified: vector magnitude is zero!");
  if (mag(ez_)<SMALL)
    throw insight::Exception("invalid direction ez specified: vector magnitude is zero!");
  
  er_/=mag(er_);
  ez_/=mag(ez_);
  e3_=ez_^er_;
  if (mag(e3_)<SMALL)
    throw insight::Exception("invalid directions er and ez specified: vector magnitude of resulting e3 is zero!");
  e3_/=mag(e3_);
}

const point& cylindrical_csys::origin() const
{
  return orig_;
}

vector cylindrical_csys::localPointToGlobal(const point& p) const
{
  double r=p.x();
  double phi=p.y();
  return orig_ + cos(phi)*r*er_ + sin(phi)*r*e3_ + p.z()*ez_;
}

vector cylindrical_csys::localVectorToGlobal(const point& p, const vector& v) const
{
  vector r=p-orig_;
  r-=(r&ez_)*ez_;
  double phi=atan2(r&e3_, r&er_);
  vector erp=cos(phi)*er_ + sin(phi)*e3_;
  vector etp=ez_^erp;
  return v.x()*erp + v.y()*etp + v.z()*ez_;
}

autoPtr<csys> cylindrical_csys::clone() const
{
  return autoPtr<csys>(new cylindrical_csys(*this));
}


}