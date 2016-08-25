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

#ifndef INSIGHT_CAD_COIL_H
#define INSIGHT_CAD_COIL_H

#include "cadfeature.h"

namespace insight {
namespace cad {


class CoilPath
    : public Feature
{

    /**
     * length of straight part of conductors
     */
    ScalarPtr l_;

    /**
     * minimal bending radius = distance between nearest opposite conductors in the center
     */
    ScalarPtr r_;

    /**
     * number of turns
     */
    ScalarPtr nr_;
    
    /**
     * distance between two subsequent conductors on the same coil side
     */
    ScalarPtr d_;

    /**
     * additional spacing at inner endwinding
     */
    ScalarPtr d0_;

    /**
     * outer yoke radius
     */
    ScalarPtr R_;


public:
    declareType("CoilPath");
    
    CoilPath(const NoParameters& nop = NoParameters());
    CoilPath
    (
        ScalarPtr l,
        ScalarPtr r,
        ScalarPtr nr,
        ScalarPtr d,
        ScalarPtr R,
     ScalarPtr d0
    );

    virtual void build();

    virtual void insertrule(parser::ISCADParser& ruleset) const;
    virtual bool isSingleEdge() const {
        return true;
    };
    virtual bool isSingleCloseWire() const;
    virtual bool isSingleOpenWire() const;
};
    
    
    
class Coil 
: public Feature
{
  VectorPtr p0_;
  VectorPtr b_, l_;
  ScalarPtr r_;
  
  /**
   * wire diameter
   */
  ScalarPtr d_;
  
  /**
   * number of layers in vertical direction
   */
  ScalarPtr nv_;

  /**
   * number of layers in radial direction
   */
  ScalarPtr nr_;
  
public:
  declareType("Coil");
  Coil(const NoParameters& nop = NoParameters());
  Coil
  (
    VectorPtr p0,
    VectorPtr b,
    VectorPtr l,
    ScalarPtr r,
    ScalarPtr d,
    ScalarPtr nv, 
    ScalarPtr nr
  );
  
  virtual void build();
  
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual bool isSingleEdge() const { return true; };
  virtual bool isSingleCloseWire() const;
  virtual bool isSingleOpenWire() const;
};

}
}

#endif // INSIGHT_CAD_COIL_H
