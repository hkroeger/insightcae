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


#ifndef INSIGHT_PREDICTINSERTIONLOCATION_H
#define INSIGHT_PREDICTINSERTIONLOCATION_H

#include <set>
#include <algorithm>
#include <iterator>

namespace insight {

template<class OrgKeyType, class KeyType = OrgKeyType>
int predictSetInsertionLocation(const std::set<OrgKeyType>& org_keys, const KeyType& newKey)
{
  std::set<KeyType> keys;
  std::transform(
        org_keys.begin(), org_keys.end(),
        std::inserter(keys, keys.begin()),
        [](const typename std::set<OrgKeyType>::value_type& i)
        {
          return static_cast<KeyType>(i);
        }
  );
  keys.insert(newKey);
  auto i=keys.find(newKey);
  return std::distance(keys.begin(), i);
}



template<class KeyType, class Container>
int predictInsertionLocation(const Container& org_data, const KeyType& newKey)
{
  std::set<KeyType> org_keys;
  // retrieve keys only
  std::transform(
        org_data.begin(), org_data.end(),
        std::inserter(org_keys, org_keys.begin()),
        [](const typename Container::value_type& i)
        {
          return static_cast<KeyType>(i.first);
        }
  );
  return predictSetInsertionLocation(org_keys, newKey);
}

}

#endif // INSIGHT_PREDICTINSERTIONLOCATION_H
