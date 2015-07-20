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

#ifndef FOAM_CSYS_H
#define FOAM_CSYS_H

#include "vector.H"
#include "point.H"
#include "dictionary.H"
#include "autoPtr.H"
#include "typeInfo.H"

namespace Foam 
{

class csys
{
public:
#ifndef SWIG
    declareRunTimeSelectionTable
    (
	autoPtr,
	csys,
	dictionary,
	(
	    const dictionary& dict
	),
	(dict)
    );
#endif
    //- Select constructed from dictionary
    static autoPtr<csys> New
    (
	const dictionary&
    );
    
    //- Runtime type information
    TypeName("csys");

    csys();
    csys(const dictionary& dict);
    virtual ~csys();
    
    virtual const point& origin() const =0;
    virtual vector localPointToGlobal(const point& p) const =0;
    virtual vector localVectorToGlobal(const point& p, const vector& v) const =0;

    virtual autoPtr<csys> clone() const =0;
};


class cartesian_csys
: public csys
{
protected:
  point orig_;
  vector ex_, ey_, ez_;
  
public:
    //- Runtime type information
    TypeName("cartesian");

    cartesian_csys();
    cartesian_csys(const dictionary& dict);
    
    virtual const point& origin() const;
    virtual vector localPointToGlobal(const point& p) const;
    virtual vector localVectorToGlobal(const point& p, const vector& v) const;

    virtual autoPtr<csys> clone() const;
};

/**
 * cylindrical coordinate system
 * local coords are (r, phi, z)
 */
class cylindrical_csys
: public csys
{
protected:
  point orig_;
  vector ez_, er_, e3_;
  
public:
    //- Runtime type information
    TypeName("cylindrical");

    cylindrical_csys();
    cylindrical_csys(const dictionary& dict);
    
    virtual const point& origin() const;
    virtual vector localPointToGlobal(const point& p) const;
    virtual vector localVectorToGlobal(const point& p, const vector& v) const;

    virtual autoPtr<csys> clone() const;
};

}

#endif // FOAM_CSYS_H
