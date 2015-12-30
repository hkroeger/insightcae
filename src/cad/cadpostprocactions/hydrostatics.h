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

#ifndef INSIGHT_CAD_HYDROSTATICS_H
#define INSIGHT_CAD_HYDROSTATICS_H

#include "cadtypes.h"
#include "cadparameters.h"
#include "cadpostprocaction.h"

namespace insight 
{
namespace cad 
{

class Hydrostatics 
: public insight::cad::PostprocAction
{
  FeaturePtr hullvolume_;
  FeaturePtr shipmodel_;
  VectorPtr psurf_;
  VectorPtr nsurf_;
  VectorPtr elong_;
  VectorPtr evert_;
    
  /**
   * lateral direction
   */
  arma::mat elat_;
  
  /**
   * mass of ship assembly
   */
  double m_;
  
  /**
   * displaced volume
   */
  double V_;
  
  /**
   * centre of buoyancy
   */
  arma::mat B_;
  
  /**
   * centre of gravity
   */
  arma::mat G_;
  
  /**
   * metacentre
   */
  arma::mat M_;
  
public:
  Hydrostatics
  (
    FeaturePtr hullvolume, 
    FeaturePtr shipmodel,
    VectorPtr psurf, 
    VectorPtr nsurf,
    VectorPtr elong, 
    VectorPtr evert
  );
  
  virtual AIS_InteractiveObject* createAISRepr() const;
  virtual void write(std::ostream& ) const;
  virtual void build();
};

}
}

#endif // INSIGHT_CAD_HYDROSTATICS_H
