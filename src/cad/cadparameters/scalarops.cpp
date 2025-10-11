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

#include "scalarops.h"

#include "parameterlisthash.h"

insight::cad::MultipliedScalar::MultipliedScalar(const MultipliedScalar &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::MultipliedScalar::MultipliedScalar(insight::cad::ScalarPtr p1, insight::cad::ScalarPtr p2)
: p1_(p1), p2_(p2)
{
    // registerDependencies({&p1_, &p2_});
}

size_t insight::cad::MultipliedScalar::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

double insight::cad::MultipliedScalar::calcValue() const
{
  return p1_->value() * p2_->value();
}




insight::cad::DividedScalar::DividedScalar(const DividedScalar &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::DividedScalar::DividedScalar(insight::cad::ScalarPtr p1, insight::cad::ScalarPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::DividedScalar::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

double insight::cad::DividedScalar::calcValue() const
{
  return p1_->value() / p2_->value();
}




insight::cad::AddedScalar::AddedScalar(const AddedScalar &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::AddedScalar::AddedScalar(insight::cad::ScalarPtr p1, insight::cad::ScalarPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::AddedScalar::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

double insight::cad::AddedScalar::calcValue() const
{
  return p1_->value() + p2_->value();
}




insight::cad::SubtractedScalar::SubtractedScalar(const SubtractedScalar &o, TreeCloneMap &tcm)
    : CL(p1_), CL(p2_)
{}

insight::cad::SubtractedScalar::SubtractedScalar(insight::cad::ScalarPtr p1, insight::cad::ScalarPtr p2)
: p1_(p1), p2_(p2)
{}

size_t insight::cad::SubtractedScalar::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=*p2_;
    return h.getHash();
}

double insight::cad::SubtractedScalar::calcValue() const
{
  return p1_->value() - p2_->value();
}



insight::cad::VectorComponent::VectorComponent(const VectorComponent &o, TreeCloneMap &tcm)
    : CL(p1_), cmpt_(o.cmpt_)
{}

insight::cad::VectorComponent::VectorComponent(insight::cad::VectorPtr p1, int cmpt)
: p1_(p1),
  cmpt_(cmpt)
{}

size_t insight::cad::VectorComponent::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    h+=cmpt_;
    return h.getHash();
}

double insight::cad::VectorComponent::calcValue() const
{
  return p1_->value()(cmpt_);
}

insight::cad::VectorMag::VectorMag(const VectorMag &o, TreeCloneMap &tcm)
    : CL(p1_)
{}

insight::cad::VectorMag::VectorMag(insight::cad::VectorPtr p1)
: p1_(p1)
{}

size_t insight::cad::VectorMag::calcHash() const
{
    ParameterListHash h;
    h+=*p1_;
    return h.getHash();
}

double insight::cad::VectorMag::calcValue() const
{
  return arma::norm(p1_->value(),2);
}

/*
insight::cad::UpperTolerance::UpperTolerance(ScalarPtr nominal, const std::string& tolstring)
: nominal_(nominal), tolstring_(tolstring)
{
}

double insight::cad::UpperTolerance::value() const
{
    double val = nominal_->value();
    
    return val;
}


insight::cad::LowerTolerance::LowerTolerance(ScalarPtr nominal, const std::string& tolstring)
: nominal_(nominal), tolstring_(tolstring)
{
}

double insight::cad::LowerTolerance::value() const
{
    double val = nominal_->value();
    
    return val;
}*/
