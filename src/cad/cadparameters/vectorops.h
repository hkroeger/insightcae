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

#ifndef INSIGHT_CAD_VECTOROPS_H
#define INSIGHT_CAD_VECTOROPS_H

#include "cadparameter.h"
#include "base/cppextensions.h"

namespace insight 
{
namespace cad 
{

class VectorFromComponents
: public insight::cad::Vector
{
  ScalarPtr p1_, p2_, p3_;


  VectorFromComponents(const VectorFromComponents&o, TreeCloneMap& tcm);
public:
  CLONEABLE(VectorFromComponents);
#ifndef SWIG
  DEPENDS((p1_, p2_, p3_));
#endif

  VectorFromComponents(ScalarPtr p1, ScalarPtr p2, ScalarPtr p3);
  arma::mat value() const override;
};




class CrossMultipliedVector
: public insight::cad::Vector
{
  VectorPtr p1_, p2_;

    CrossMultipliedVector(const CrossMultipliedVector&o, TreeCloneMap& tcm);
public:
    CLONEABLE(CrossMultipliedVector);
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif

  CrossMultipliedVector(VectorPtr p1, VectorPtr p2);
  arma::mat value() const override;
};




class DotMultipliedVector
: public insight::cad::Scalar
{
  VectorPtr p1_, p2_;
    DotMultipliedVector(const DotMultipliedVector&o, TreeCloneMap& tcm);
public:
    CLONEABLE(DotMultipliedVector);
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif

  DotMultipliedVector(VectorPtr p1, VectorPtr p2);
  double value() const override;
};




class ScalarMultipliedVector
: public insight::cad::Vector
{
  ScalarPtr p1_;
  VectorPtr p2_;

  ScalarMultipliedVector(const ScalarMultipliedVector&o, TreeCloneMap& tcm);
public:
  CLONEABLE(ScalarMultipliedVector);
#ifndef SWIG
  DEPENDS((p1_, p2_));
#endif

  ScalarMultipliedVector(ScalarPtr p1, VectorPtr p2);
  arma::mat value() const override;
};




class ScalarDividedVector
: public insight::cad::Vector
{
  VectorPtr p1_;
  ScalarPtr p2_;
  ScalarDividedVector(const ScalarDividedVector&o, TreeCloneMap& tcm);
public:
  CLONEABLE(ScalarDividedVector);
#ifndef SWIG
  DEPENDS((p1_, p2_));
#endif

  ScalarDividedVector(VectorPtr p1, ScalarPtr p2);
  arma::mat value() const override;
};




class AddedVector
: public insight::cad::Vector
{
  VectorPtr p1_, p2_;

    AddedVector(const AddedVector&o, TreeCloneMap& tcm);
public:
    CLONEABLE(AddedVector);
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif

  AddedVector(VectorPtr p1, VectorPtr p2);

  inline VectorPtr& p1() __attribute__((deprecated("using this might break updating!")))
  { return p1_; }

  inline VectorPtr& p2() __attribute__((deprecated("using this might break updating!")))
  { return p2_; }

  arma::mat value() const override;
};



class SubtractedVector
: public insight::cad::Vector
{
  VectorPtr p1_, p2_;

    SubtractedVector(const SubtractedVector&o, TreeCloneMap& tcm);
public:
    CLONEABLE(SubtractedVector);
#ifndef SWIG
    DEPENDS((p1_, p2_));
#endif

  SubtractedVector(VectorPtr p1, VectorPtr p2);
  arma::mat value() const override;
};


class RotatedVector
: public insight::cad::Vector
{
  ScalarPtr ang_;
  VectorPtr v_, ax_;

  RotatedVector(const RotatedVector&o, TreeCloneMap& tcm);
public:
  CLONEABLE(RotatedVector);
#ifndef SWIG
  DEPENDS((ang_, v_, ax_));
#endif

  RotatedVector(VectorPtr v, ScalarPtr ang, VectorPtr ax);
  arma::mat value() const override;
};


class NormalizedVector
    : public insight::cad::Vector
{
  VectorPtr v_;

    NormalizedVector(const NormalizedVector&o, TreeCloneMap& tcm);
public:
    CLONEABLE(NormalizedVector);
#ifndef SWIG
    DEPENDS((v_));
#endif

  NormalizedVector(VectorPtr v);
  arma::mat value() const override;
};


class Mechanism_CrankDrive
: public insight::cad::Vector
{
  ScalarPtr L_;
  VectorPtr c2_; 
  ScalarPtr r2_;
  VectorPtr p1_, eax_;

  Mechanism_CrankDrive(const Mechanism_CrankDrive&o, TreeCloneMap& tcm);
public:
  CLONEABLE(Mechanism_CrankDrive);
#ifndef SWIG
  DEPENDS((L_, c2_, r2_, p1_, eax_));
#endif

  /**
  * @L con rod length
  * @c2 crank shaft center
  * @r2 crank pin radius
  * @p1 upper con rod eye location
  * @eax direction of crank shaft axis
  */
  Mechanism_CrankDrive(ScalarPtr L, VectorPtr c2, ScalarPtr r2, VectorPtr p1, VectorPtr eax);
  arma::mat value() const override;
};


class Mechanism_Slider
: public insight::cad::Vector
{
  ScalarPtr L_;
  VectorPtr p0_; 
  VectorPtr psl_; 
  VectorPtr esl_;
  Mechanism_Slider(const Mechanism_Slider&o, TreeCloneMap& tcm);
public:
  CLONEABLE(Mechanism_Slider);
#ifndef SWIG
  DEPENDS((L_, p0_, psl_, esl_));
#endif


  Mechanism_Slider(ScalarPtr L, VectorPtr p0, VectorPtr psl, VectorPtr esl);
  arma::mat value() const override;
};


}
}

#endif // INSIGHT_CAD_VECTOROPS_H
