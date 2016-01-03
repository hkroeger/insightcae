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

#ifndef INSIGHT_CAD_SCALAROPS_H
#define INSIGHT_CAD_SCALAROPS_H

#include "cadparameter.h"

namespace insight {
namespace cad {

class MultipliedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
public:
  MultipliedScalar(ScalarPtr p1, ScalarPtr p2);
  virtual double value() const;
};




class DividedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
public:
  DividedScalar(ScalarPtr p1, ScalarPtr p2);
  virtual double value() const;
};




class AddedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
public:
  AddedScalar(ScalarPtr p1, ScalarPtr p2);
  virtual double value() const;
};



class SubtractedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
public:
  SubtractedScalar(ScalarPtr p1, ScalarPtr p2);
  virtual double value() const;
};



class VectorComponent
: public insight::cad::Scalar
{
  VectorPtr p1_;
  int cmpt_;
  
public:
  VectorComponent(VectorPtr p1, int cmpt);
  virtual double value() const;
};

#define UNARY_FUNCTION(FUNCTION) \
class Scalar_##FUNCTION\
: public insight::cad::Scalar\
{\
  ScalarPtr p1_;\
public:\
  Scalar_##FUNCTION(ScalarPtr p1)\
  : p1_(p1) {} \
  virtual double value() const\
  { return ::FUNCTION ( p1_->value() ); }\
};

UNARY_FUNCTION(sqrt);
UNARY_FUNCTION(sin);
UNARY_FUNCTION(cos);
UNARY_FUNCTION(tan);
UNARY_FUNCTION(asin);
UNARY_FUNCTION(acos);
UNARY_FUNCTION(atan);

#define BINARY_FUNCTION(FUNCTION) \
class Scalar_##FUNCTION\
: public insight::cad::Scalar\
{\
  ScalarPtr p1_, p2_;\
public:\
  Scalar_##FUNCTION(ScalarPtr p1, ScalarPtr p2)\
  : p1_(p1), p2_(p2) {} \
  virtual double value() const\
  { return ::FUNCTION ( p1_->value(), p2_->value() ); }\
};

BINARY_FUNCTION(pow);
BINARY_FUNCTION(atan2);

}
}

#endif // INSIGHT_CAD_SCALAROPS_H
