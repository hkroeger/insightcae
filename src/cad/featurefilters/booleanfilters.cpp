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

#include "booleanfilters.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
AND::AND(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}

void AND::initialize(FeaturePtr m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

void AND::firstPass(FeatureID feature)
{
  Filter::firstPass(feature);
  f1_->firstPass(feature);
  f2_->firstPass(feature);
}


bool AND::checkMatch(FeatureID feature) const
{
  return f1_->checkMatch(feature) && f2_->checkMatch(feature);
}

FilterPtr AND::clone() const
{
  return FilterPtr(new AND(*f1_, *f2_));
}



OR::OR(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}

void OR::initialize(FeaturePtr m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

void OR::firstPass(FeatureID feature)
{
  Filter::firstPass(feature);
  f1_->firstPass(feature);
  f2_->firstPass(feature);
}

bool OR::checkMatch(FeatureID feature) const
{
//   cout<<feature<<" -> "<<f1_->checkMatch(feature) <<"||"<< f2_->checkMatch(feature)<<endl;
  return f1_->checkMatch(feature) || f2_->checkMatch(feature);
}

FilterPtr OR::clone() const
{
  return FilterPtr(new OR(*f1_, *f2_));
}



NOT::NOT(const Filter& f1)
: Filter(),
  f1_(f1.clone())
{
}
void NOT::initialize(FeaturePtr m)
{
  Filter::initialize(m);
  f1_->initialize(m);
}

void NOT::firstPass(FeatureID feature)
{
    Filter::firstPass(feature);
    f1_->firstPass(feature);
}


bool NOT::checkMatch(FeatureID feature) const
{
  bool ok=f1_->checkMatch(feature);
  std::cout<<"NOT="<<ok<<std::endl;
  return !ok;
}

FilterPtr NOT::clone() const
{
  return FilterPtr(new NOT(*f1_));
}

FilterPtr Filter::operator&&(const Filter& f2)
{
  return FilterPtr(new AND(*this, f2));
}

FilterPtr Filter::operator!()
{
  return FilterPtr(new NOT(*this));
}

}
}