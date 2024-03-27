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

#include "vectorspacebase.h"

namespace Foam
{




VectorSpaceBase::~VectorSpaceBase()
{}

void VectorSpaceBase::read(Istream& is)
{
  is >> p0_;
}

void VectorSpaceBase::writeSup(Ostream& os) const
{
  os << p0_;
}




void LinearVectorSpaceBase::read(Istream& is)
{
    VectorSpaceBase::read(is);
    is >> ep_;
}

void LinearVectorSpaceBase::writeSup(Ostream& os) const
{
    VectorSpaceBase::writeSup(os);
    os << token::SPACE << ep_;
}

scalar LinearVectorSpaceBase::t(const point& p) const
{
    return (p-origin())&ep_;
}




void CylCoordVectorSpaceBase::read(Istream &is)
{
    VectorSpaceBase::read(is);
    is >> eax_; eax_/=mag(eax_);
}

void CylCoordVectorSpaceBase::writeSup(Ostream &os) const
{
    VectorSpaceBase::writeSup(os);
    os << token::SPACE << eax_;
}




scalar RadialCylCoordVectorSpaceBase::t(const point& p) const
{
    vector r=(p-origin());
    r-=ax()*(r&ax());
    return mag(r);
}




void CircumCylCoordVectorSpaceBase::read(Istream&is)
{
    CylCoordVectorSpaceBase::read(is);
    is >> ephi0_org_;
    vector ey = ax()^ephi0_org_;
    ephi0_=ey^ax(); ephi0_=ephi0_/mag(ephi0_);
}

void CircumCylCoordVectorSpaceBase::writeSup(Ostream& os) const
{
    CylCoordVectorSpaceBase::writeSup(os);
    os << token::SPACE << ephi0_org_;
}

scalar CircumCylCoordVectorSpaceBase::t(const point& p) const
{
    vector r=(p-origin());
    r-=ax()*(r&ax());
    double x=r&ephi0_;
    double y=r&(ax()^ephi0_);
    return atan2(y, x);
}

}
