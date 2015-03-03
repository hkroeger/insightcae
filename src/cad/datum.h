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

#ifndef INSIGHT_CAD_DATUM_H
#define INSIGHT_CAD_DATUM_H

#include <map>

#include "base/linearalgebra.h"

#include "occinclude.h"
// #include "solidmodel.h"
#include "cadtypes.h"

namespace insight {
namespace cad {

class Datum
{
public:
  typedef std::map<std::string, DatumPtr> Map;
  
protected:
  bool providesPointReference_, providesAxisReference_, providesPlanarReference_;
  
public:
  Datum(bool point, bool axis, bool planar);
  virtual ~Datum();
  
  inline bool providesPointReference() const { return providesPointReference_; }
  virtual operator const gp_Pnt () const;
  
  inline bool providesAxisReference() const { return providesAxisReference_; }
  virtual operator const gp_Ax1 () const;

  inline bool providesPlanarReference() const { return providesPlanarReference_; }
  virtual operator const gp_Ax3 () const;

  virtual AIS_InteractiveObject* createAISRepr() const;

};

class DatumPlane
: public Datum
{
  gp_Ax3 cs_;
  
public:
  DatumPlane
  (
    const arma::mat& p0, 
    const arma::mat& n
  );
  DatumPlane
  (
    const arma::mat& p0, 
    const arma::mat& n,
    const arma::mat& up
  );
  DatumPlane
  (
    const SolidModel& m, 
    FeatureID f
  );
  
  virtual operator const gp_Pnt () const;
  virtual operator const gp_Ax3 () const;

  virtual AIS_InteractiveObject* createAISRepr() const;
};

}
}

#endif // INSIGHT_CAD_DATUM_H
