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

#include <tuple>
#include "parameterlisthash.h"
#include "base/cppextensions.h"

namespace insight {
namespace cad {

class MultipliedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
  MultipliedScalar(const MultipliedScalar&o, TreeCloneMap& tcm);
public:
  declareType("MultipliedScalar");
#ifndef SWIG
    DEPENDS((p1_, p2_))
#endif
  CLONEABLE(MultipliedScalar);

  MultipliedScalar(ScalarPtr p1, ScalarPtr p2);
   size_t calcHash() const override;
  double calcValue() const override;
};




class DividedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
    DividedScalar(const DividedScalar&o, TreeCloneMap& tcm);
public:
    declareType("DividedScalar");
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif
  CLONEABLE(DividedScalar);

  DividedScalar(ScalarPtr p1, ScalarPtr p2);
   size_t calcHash() const override;
  double calcValue() const override;
};




class AddedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
    AddedScalar(const AddedScalar&o, TreeCloneMap& tcm);
public:
    declareType("AddedScalar");
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif
  CLONEABLE(AddedScalar);

  AddedScalar(ScalarPtr p1, ScalarPtr p2);
   size_t calcHash() const override;
  double calcValue() const override;
};



class SubtractedScalar
: public insight::cad::Scalar
{
  ScalarPtr p1_, p2_;
    SubtractedScalar(const SubtractedScalar&o, TreeCloneMap& tcm);
public:
    declareType("SubtractedScalar");
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif
  CLONEABLE(SubtractedScalar);

  SubtractedScalar(ScalarPtr p1, ScalarPtr p2);
   size_t calcHash() const override;
  double calcValue() const override;
};



class VectorComponent
: public insight::cad::Scalar
{
  VectorPtr p1_;
  int cmpt_;
  
  VectorComponent(const VectorComponent&o, TreeCloneMap& tcm);
public:
  declareType("VectorComponent");
#ifndef SWIG
  DEPENDS((p1_));
#endif
  CLONEABLE(VectorComponent);

  VectorComponent(VectorPtr p1, int cmpt);
   size_t calcHash() const override;
  double calcValue() const override;
};



class VectorMag
: public insight::cad::Scalar
{
  VectorPtr p1_;

  VectorMag(const VectorMag&o, TreeCloneMap& tcm);
public:
  declareType("VectorMag");
#ifndef SWIG
  DEPENDS((p1_));
#endif
  CLONEABLE(VectorMag);

  VectorMag(VectorPtr p1);
   size_t calcHash() const override;
  double calcValue() const override;
};



#define INSIGHT_CAD_UNARY_FUNCTION_WITH_NAME(FUNCTION, NAME) \
class Scalar_##NAME\
: public insight::cad::Scalar\
{\
  ScalarPtr p1_;\
  Scalar_##NAME(const Scalar_##NAME&o, TreeCloneMap& tcm)\
  : CL(p1_) \
  {}\
public:\
  DEPENDS((p1_));\
  CLONEABLE(Scalar_##NAME);\
  Scalar_##NAME(ScalarPtr p1)\
  : p1_(p1) {} \
 size_t calcHash() const override\
{ ParameterListHash h; h+=*p1_; return h.getHash(); }\
  double calcValue() const override\
  { return ::FUNCTION ( p1_->value() ); }\
};

#define INSIGHT_CAD_UNARY_FUNCTION(FUNCTION) \
INSIGHT_CAD_UNARY_FUNCTION_WITH_NAME(FUNCTION, FUNCTION)

INSIGHT_CAD_UNARY_FUNCTION_WITH_NAME(insight::pos, pos);
INSIGHT_CAD_UNARY_FUNCTION_WITH_NAME(insight::neg, neg);
INSIGHT_CAD_UNARY_FUNCTION_WITH_NAME(insight::sgn, sgn);
INSIGHT_CAD_UNARY_FUNCTION(sqrt);
INSIGHT_CAD_UNARY_FUNCTION(sin);
INSIGHT_CAD_UNARY_FUNCTION(cos);
INSIGHT_CAD_UNARY_FUNCTION(tan);
INSIGHT_CAD_UNARY_FUNCTION(asin);
INSIGHT_CAD_UNARY_FUNCTION(acos);
INSIGHT_CAD_UNARY_FUNCTION(atan);
INSIGHT_CAD_UNARY_FUNCTION(ceil);
INSIGHT_CAD_UNARY_FUNCTION(floor);
INSIGHT_CAD_UNARY_FUNCTION(round);

#define INSIGHT_CAD_BINARY_FUNCTION(FUNCTION) \
class Scalar_##FUNCTION\
: public insight::cad::Scalar\
{\
  ScalarPtr p1_, p2_;\
  Scalar_##FUNCTION(const Scalar_##FUNCTION&o, TreeCloneMap& tcm)\
    : CL(p1_), CL(p2_) \
  {}\
public:\
  DEPENDS((p1_, p2_)); \
  CLONEABLE(Scalar_##FUNCTION);\
  Scalar_##FUNCTION(ScalarPtr p1, ScalarPtr p2)\
  : p1_(p1), p2_(p2) {} \
  size_t calcHash() const override \
  { ParameterListHash h; h+=*p1_; h+=*p2_; return h.getHash(); } \
  double calcValue() const override\
  { return ::FUNCTION ( p1_->value(), p2_->value() ); }\
};

INSIGHT_CAD_BINARY_FUNCTION(pow);
INSIGHT_CAD_BINARY_FUNCTION(atan2);


// class UpperTolerance
// : public insight::cad::Scalar
// {
//     ScalarPtr nominal_;
//     std::string tolstring_;
// public:
//     UpperTolerance(ScalarPtr nominal, const std::string& tolstring);
//     virtual double value() const;
// };
// 
// class LowerTolerance
// : public insight::cad::Scalar
// {
//     ScalarPtr nominal_;
//     std::string tolstring_;
// public:
//     LowerTolerance(ScalarPtr nominal, const std::string& tolstring);
//     virtual double value() const;
// };

}
}

#endif // INSIGHT_CAD_SCALAROPS_H
