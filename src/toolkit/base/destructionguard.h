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


#ifndef INSIGHT_DESTRUCTIONGUARD_H
#define INSIGHT_DESTRUCTIONGUARD_H

#include <functional>
#include <utility>

namespace insight {

/**
 * wrapper for calling virtual functions before destruction
 */
template <typename T>
class DestructionGuard : public T
{
  std::function<void()> preDestruction_;

public:
  template<class ...Args>
  DestructionGuard(Args&&... addArgs)
   : T(std::forward<Args>(addArgs)...),
     preDestruction_([](){})
  {}

  void setPreDestructionFunction(std::function<void()> preDestruction)
  {
      preDestruction_=preDestruction;
  }

  ~DestructionGuard()
  {
     preDestruction_();
     // now T is destructed
  }
};

}

#endif // INSIGHT_DESTRUCTIONGUARD_H
