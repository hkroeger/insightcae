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
 *
 */

#include "feature.h"
#include "solidmodel.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
FeatureSet::FeatureSet(const SolidModel& m, TopAbs_ShapeEnum shape)
: model_(m),
  shape_(shape)
{
}

void FeatureSet::safe_union(const FeatureSet& o)
{
  if (o.shape_!=shape_)
    throw insight::Exception("incompatible shape type between feature sets!");
  else if (!(o.model_==model_))
    throw insight::Exception("feature sets belong to different models!");
  else
  {
    insert(o.begin(), o.end());
  }
}


std::auto_ptr<FeatureSet> FeatureSet::clone() const
{
  std::auto_ptr<FeatureSet> nfs(new FeatureSet(model_, shape_));
  nfs->insert(begin(), end());
  return nfs;
}


  
}
}