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

#ifndef INSIGHT_CAD_POINTERTRANSIENT_H
#define INSIGHT_CAD_POINTERTRANSIENT_H

#include "Standard_Version.hxx"
#include "Standard_OStream.hxx"
#if (OCC_VERSION_MAJOR<7)
#include "Standard_Transient_proto.hxx"
#else
#include "Standard_Transient.hxx"
#endif

class QObject;

class PointerTransient 
: public Standard_Transient
{
protected:
  QObject* mi_;
  
public:
  PointerTransient();
  PointerTransient(const PointerTransient& o);
  PointerTransient(QObject* mi);
  ~PointerTransient();
  
  void operator=(QObject* mi);
  QObject* getPointer();
    
};


#endif // INSIGHT_CAD_POINTERTRANSIENT_H
