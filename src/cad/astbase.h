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

#ifndef INSIGHT_CAD_ASTBASE_H
#define INSIGHT_CAD_ASTBASE_H

namespace insight
{
namespace cad
{

/**
 * implements update-on-request framework
 */
class ASTBase
{
  bool valid_;
  mutable bool building_;
  
public:
  ASTBase();
  virtual ~ASTBase();
  
  inline void setValid() 
  { 
    valid_=true; 
  }
  
  inline bool valid() const 
  { 
    return valid_; 
  }

  inline bool building() const 
  { 
    return building_; 
  }

  void checkForBuildDuringAccess() const;
  virtual void build() =0;

};


}
}

#endif // INSIGHT_CAD_ASTBASE_H
