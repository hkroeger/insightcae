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

#include "vectorops.h"

insight::cad::VectorFromComponents::VectorFromComponents
(
  insight::cad::ScalarPtr p1,
  insight::cad::ScalarPtr p2, 
  insight::cad::ScalarPtr p3
)
: p1_(p1), p2_(p2), p3_(p3)
{

}

arma::mat insight::cad::VectorFromComponents::value() const
{
  return vec3(p1_->value(), p2_->value(), p3_->value());
}




insight::cad::CrossMultipliedVector::CrossMultipliedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

arma::mat insight::cad::CrossMultipliedVector::value() const
{
  return arma::cross(p1_->value(), p2_->value());
}




insight::cad::DotMultipliedVector::DotMultipliedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

double insight::cad::DotMultipliedVector::value() const
{
  return arma::dot(p1_->value(), p2_->value());
}



insight::cad::ScalarMultipliedVector::ScalarMultipliedVector(insight::cad::ScalarPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

arma::mat insight::cad::ScalarMultipliedVector::value() const
{
  return p1_->value()*p2_->value();
}



insight::cad::ScalarDividedVector::ScalarDividedVector(insight::cad::VectorPtr p1, insight::cad::ScalarPtr p2)
: p1_(p1), p2_(p2)
{}

arma::mat insight::cad::ScalarDividedVector::value() const
{
  return p1_->value() / p2_->value();
}



insight::cad::AddedVector::AddedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

arma::mat insight::cad::AddedVector::value() const
{
  return p1_->value() + p2_->value();
}



insight::cad::SubtractedVector::SubtractedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

arma::mat insight::cad::SubtractedVector::value() const
{
  return p1_->value() - p2_->value();
}


insight::cad::RotatedVector::RotatedVector
(
  insight::cad::VectorPtr v, 
  insight::cad::ScalarPtr ang, 
  insight::cad::VectorPtr ax
)
: v_(v), ang_(ang), ax_(ax)
{}


arma::mat insight::cad::RotatedVector::value() const
{
  return rotMatrix(ang_->value(), ax_->value()) * v_->value();
}
