 
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

#ifndef INSIGHT_CAD_DERIVEDFEATURE_H
#define INSIGHT_CAD_DERIVEDFEATURE_H

#include "cadparameters.h"
#include "cadfeature.h"

namespace insight 
{
namespace cad
{

class DerivedFeature
: public Feature
{
  ConstFeaturePtr basefeat_;
  
public:
  declareType("DerivedFeature");
  
  DerivedFeature(const NoParameters& nop = NoParameters());
  DerivedFeature(ConstFeaturePtr basefeat);
  
  inline void setBaseFeat(ConstFeaturePtr basefeat) { basefeat_=basefeat; }
  
  virtual double density() const;
  virtual double areaWeight() const;
  virtual arma::mat modelCoG() const;
  virtual arma::mat modelInertia() const;
  virtual double mass(double density_ovr=-1., double aw_ovr=-1.) const;
};


}
}

#endif // INSIGHT_CAD_ARC_H
