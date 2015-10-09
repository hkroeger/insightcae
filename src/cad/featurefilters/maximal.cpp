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

#include "maximal.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
maximal::maximal(const scalarQuantityComputer& qtc, int rank)
: qtc_(qtc.clone()),
  rank_(rank)
{
}

void maximal::initialize(const SolidModel& m)
{
  Filter::initialize(m);
  qtc_->initialize(m);
}

void maximal::firstPass(FeatureID feature)
{
  if (qtc_->isValidForFeature(feature))
    ranking_[-qtc_->evaluate(feature)].insert(feature);
}

bool maximal::checkMatch(FeatureID feature) const
{
  int j=0;
  for (std::map<double, std::set<FeatureID> >::const_iterator i=ranking_.begin(); i!=ranking_.end(); i++)
  {
    if ((i->second.find(feature)!=i->second.end())) 
    {
      std::cout<<"Feature #"<<feature<<" rank="<<j<<" match="<<(j==rank_)<<std::endl;
    }
    if ((i->second.find(feature)!=i->second.end()) && (j==rank_)) 
    {
      return true;
    }
    j++;
  }
  return false;
}

FilterPtr maximal::clone() const
{
  return FilterPtr(new maximal(*qtc_, rank_));
}

}
}