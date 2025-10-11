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
#include "parameterlisthash.h"
#include "cadfeature.h"
#include "datum.h"

insight::cad::VectorFromComponents::VectorFromComponents(const VectorFromComponents &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_), CL(p3_)
{}

insight::cad::VectorFromComponents::VectorFromComponents
(
  insight::cad::ScalarPtr p1,
  insight::cad::ScalarPtr p2, 
  insight::cad::ScalarPtr p3
)
: p1_(p1), p2_(p2), p3_(p3)
{

}

size_t insight::cad::VectorFromComponents::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    h+=*p3_;
    return h.getHash();
}

arma::mat insight::cad::VectorFromComponents::calcValue() const
{
  return vec3(p1_->value(), p2_->value(), p3_->value());
}




insight::cad::CrossMultipliedVector::CrossMultipliedVector(const CrossMultipliedVector &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::CrossMultipliedVector::CrossMultipliedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::CrossMultipliedVector::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

arma::mat insight::cad::CrossMultipliedVector::calcValue() const
{
  return arma::cross(p1_->value(), p2_->value());
}




insight::cad::DotMultipliedVector::DotMultipliedVector(const DotMultipliedVector &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::DotMultipliedVector::DotMultipliedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::DotMultipliedVector::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

double insight::cad::DotMultipliedVector::calcValue() const
{
  return arma::dot(p1_->value(), p2_->value());
}



insight::cad::ScalarMultipliedVector::ScalarMultipliedVector(const ScalarMultipliedVector &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::ScalarMultipliedVector::ScalarMultipliedVector(insight::cad::ScalarPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::ScalarMultipliedVector::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

arma::mat insight::cad::ScalarMultipliedVector::calcValue() const
{
  return p1_->value()*p2_->value();
}



insight::cad::ScalarDividedVector::ScalarDividedVector(const ScalarDividedVector &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::ScalarDividedVector::ScalarDividedVector(insight::cad::VectorPtr p1, insight::cad::ScalarPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::ScalarDividedVector::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

arma::mat insight::cad::ScalarDividedVector::calcValue() const
{
  return p1_->value() / p2_->value();
}



insight::cad::AddedVector::AddedVector(const AddedVector &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::AddedVector::AddedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::AddedVector::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

arma::mat insight::cad::AddedVector::calcValue() const
{
  return p1_->value() + p2_->value();
}



insight::cad::SubtractedVector::SubtractedVector(const SubtractedVector &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::SubtractedVector::SubtractedVector(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::SubtractedVector::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

arma::mat insight::cad::SubtractedVector::calcValue() const
{
  return p1_->value() - p2_->value();
}


insight::cad::RotatedVector::RotatedVector(const RotatedVector &o, TreeCloneMap &tcm)
    : CL(ang_), CL(v_), CL(ax_)
{}

insight::cad::RotatedVector::RotatedVector
(
  insight::cad::VectorPtr v, 
  insight::cad::ScalarPtr ang, 
  insight::cad::VectorPtr ax
)
: v_(v), ang_(ang), ax_(ax)
{}

size_t insight::cad::RotatedVector::calcHash() const
{
    ParameterListHash h;
    h+=*v_;
    h+=*ang_;
    h+=*ax_;
    return h.getHash();
}

arma::mat insight::cad::RotatedVector::calcValue() const
{
  return rotMatrix(ang_->value(), ax_->value()) * v_->value();
}




insight::cad::NormalizedVector::NormalizedVector(const NormalizedVector &o, TreeCloneMap &tcm)
    : CL(v_)
{}

insight::cad::NormalizedVector::NormalizedVector(VectorPtr v)
    : v_(v)
{}


size_t insight::cad::NormalizedVector::calcHash() const
{
    ParameterListHash h;
    h+=*v_;
    return h.getHash();
}

arma::mat insight::cad::NormalizedVector::calcValue() const
{
  return insight::normalized(v_->value());
}


insight::cad::Mechanism_CrankDrive::Mechanism_CrankDrive(const Mechanism_CrankDrive &o, TreeCloneMap &tcm)
    : CL(L_), CL(c2_), CL(r2_), CL(p1_), CL(eax_)
{}

insight::cad::Mechanism_CrankDrive::Mechanism_CrankDrive(ScalarPtr L, VectorPtr c2, ScalarPtr r2, VectorPtr p1, VectorPtr eax)
: L_(L), c2_(c2), r2_(r2), p1_(p1), eax_(eax)
{
}

size_t insight::cad::Mechanism_CrankDrive::calcHash() const
{
    ParameterListHash h;
    h+=*L_;
    h+=*c2_;
    h+=*r2_;
    h+=*p1_;
    h+=*eax_;
    return h.getHash();
}

arma::mat insight::cad::Mechanism_CrankDrive::calcValue() const
{
  arma::mat edelta = c2_->value() - p1_->value();
  arma::mat eax=eax_->value();
  eax/=arma::norm(eax,2);
  double delta=arma::norm(edelta,2);
  edelta /= delta;
  double phi = 
   acos(
     ( pow(delta,2) + pow(L_->value(),2) - pow(r2_->value(),2) )
     /
     ( 2.*delta*L_->value() )
   );
   
  std::cout<<"phi="<<phi<<std::endl;
   
  return p1_->value() + L_->value()*(rotMatrix(phi, eax)*edelta);
}

insight::cad::Mechanism_Slider::Mechanism_Slider(const Mechanism_Slider &o, TreeCloneMap &tcm)
: CL(L_), CL(p0_), CL(psl_), CL(esl_)
{}

insight::cad::Mechanism_Slider::Mechanism_Slider(ScalarPtr L, VectorPtr p0, VectorPtr psl, VectorPtr esl)
: L_(L), p0_(p0), psl_(psl), esl_(esl)
{}

size_t insight::cad::Mechanism_Slider::calcHash() const
{
    ParameterListHash h;
    h+=*L_;
    h+=*p0_;
    h+=*psl_;
    h+=*esl_;
    return h.getHash();
}

arma::mat insight::cad::Mechanism_Slider::calcValue() const
{
  arma::mat edelta=p0_->value()-psl_->value();
  double delta=arma::norm(edelta,2);
  edelta/=delta;
  arma::mat esl=esl_->value();
  esl/=arma::norm(esl,2);
  double cosa=arma::dot(edelta, esl);
  double r=L_->value();
  
  double f1=cosa*delta;
  double f2=sqrt(r*r-delta*delta+pow(cosa*delta,2));
  std::cout<<"f1="<<f1<<", f2="<<f2<<std::endl;
  
  double x1=f1+f2;
  double x2=f1-f2;
  std::cout<<"x1="<<x1<<", x2="<<x2<<std::endl;
  if (fabs(x1)<fabs(x2))
  {
    return psl_->value() + x1*esl;
  }
  else
  {
    return psl_->value() + x2*esl;
  }
}

