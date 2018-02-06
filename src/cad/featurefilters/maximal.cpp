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
  
maximal::maximal(const scalarQuantityComputer& qtc, int rank, int lrank)
: qtc_(qtc.clone()),
  rank_(rank), lrank_(std::max(rank,lrank))
{
}

void maximal::initialize(ConstFeaturePtr m)
{
  Filter::initialize(m);
  qtc_->initialize(m);
}

void maximal::firstPass(FeatureID feature)
{
  if (qtc_->isValidForFeature(feature))
    {
      ranking_.push_back(RankEntry(-qtc_->evaluate(feature), feature));
      std::sort( ranking_.begin(), ranking_.end(), [](const RankEntry& e1,const RankEntry& e2) -> bool
       { return e1.first < e2.first; }
      );
    }
}

bool maximal::checkMatch(FeatureID feature) const
{
  int il=ranking_.size()-1;
  if (il>0)
    {
      for (int j=std::max(0, std::min(il, rank_)); j<=std::max(0, std::min(il, lrank_)); j++)
      {
          const RankEntry& e = ranking_[j];
          if (e.second==feature)
          {
            std::cout<<"Feature #"<<feature<<" rank="<<j<<" match!"<<std::endl;
            return true;
          }
      }
    }
  return false;
}

FilterPtr maximal::clone() const
{
  return FilterPtr(new maximal(*qtc_, rank_, lrank_));
}

}
}
