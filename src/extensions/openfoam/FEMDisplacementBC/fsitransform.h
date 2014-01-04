/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef FOAM_FSITRANSFORM_H
#define FOAM_FSITRANSFORM_H

#include "fvCFD.H"
#include "typeInfo.H"
#include "sphericalTensor.H"

namespace Foam {

class FSITransform
{
public:
  TypeName("FSITransform");
  
  declareRunTimeSelectionTable
  (
      autoPtr,
      FSITransform,
      dictionary,
      (
	  const dictionary& dict
      ),
      (dict)
  );
  
  FSITransform();
  FSITransform(const dictionary& dict);
  virtual ~FSITransform();
  
  static autoPtr<FSITransform> New(const dictionary& dict);
  
  virtual point locationCFDtoFEM(const point& pCFD) const =0;
  virtual vector vectorFEMtoCFD(const point& pCFD, const vector& u) const =0;
  virtual vector vectorCFDtoFEM(const point& pCFD, const vector& u) const =0;
  
  virtual void writeEntry(Ostream& dict) const;

  virtual FSITransform* clone() const =0;
};

class IdentityTransform
: public FSITransform
{
public:
  TypeName("identity");
  IdentityTransform();
  IdentityTransform(const dictionary& dict);

  virtual point locationCFDtoFEM(const point& pCFD) const;
  virtual vector vectorFEMtoCFD(const point& pCFD, const vector& u) const;
  virtual vector vectorCFDtoFEM(const point& pCFD, const vector& u) const;
  
  virtual FSITransform* clone() const;
};

}

#endif // FOAM_FSITRANSFORM_H
