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

#include "Standard_Type.hxx"

#include "pointertransient.h"
#include <QObject>

PointerTransient::PointerTransient()
: mi_(NULL)
{}

PointerTransient::PointerTransient(const PointerTransient& o)
: mi_(o.mi_)
{}

PointerTransient::PointerTransient(QObject* mi)
: mi_(mi)
{}

PointerTransient::~PointerTransient()
{}

void PointerTransient::operator=(QObject* mi)
{
  mi_=mi;
}

QObject *PointerTransient::getPointer()
{
  return mi_;
}

#if OCC_VERSION_MAJOR<7
IMPLEMENT_STANDARD_TYPE(PointerTransient)
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY()
  STANDARD_TYPE(Standard_Transient),
IMPLEMENT_STANDARD_SUPERTYPE_ARRAY_END()
IMPLEMENT_STANDARD_TYPE_END(PointerTransient)


IMPLEMENT_DOWNCAST(PointerTransient,Standard_Transient)
IMPLEMENT_STANDARD_RTTI(PointerTransient)
#endif
