/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_CAD_DATUM_H
#define INSIGHT_CAD_DATUM_H

#include "base/linearalgebra.h"

#include "occinclude.h"
#include "solidmodel.h"

namespace insight {
namespace cad {
  
class Datum
{
public:
  typedef boost::shared_ptr<Datum> Ptr;
  
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
    const SolidModel& m, 
    FeatureID f
  );
  
  virtual operator const gp_Pnt () const;
  virtual operator const gp_Ax3 () const;
};

}
}

#endif // INSIGHT_CAD_DATUM_H
