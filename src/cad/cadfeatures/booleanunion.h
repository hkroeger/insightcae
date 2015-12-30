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

#ifndef INSIGHT_CAD_BOOLEANUNION_H
#define INSIGHT_CAD_BOOLEANUNION_H

#include "cadparameters.h"
#include "cadfeature.h"

namespace insight 
{
namespace cad 
{

class BooleanUnion
: public Feature
{
  FeaturePtr m1_, m2_;
  
public:
  declareType("BooleanUnion");
  BooleanUnion(const NoParameters& nop = NoParameters());
  BooleanUnion(FeaturePtr m1);
  BooleanUnion(FeaturePtr m1, FeaturePtr m2);
  
  virtual void build();

  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

FeaturePtr operator|(FeaturePtr m1, FeaturePtr m2);

}
}

#endif // INSIGHT_CAD_BOOLEANUNION_H
