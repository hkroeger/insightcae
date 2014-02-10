/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#ifndef INSIGHT_EXPLICITVORTEX_H
#define INSIGHT_EXPLICITVORTEX_H

#include "openfoam/openfoamanalysis.h"

namespace insight {

class ExplicitVortex 
: public insight::OpenFOAMAnalysis
{
protected:
  bool stopFlag_;
public:
  declareType("Explicit Vortex");
  
  ExplicitVortex(const NoParameters&);
  ~ExplicitVortex();
  
  virtual void cancel();
  virtual ResultSetPtr operator()(ProgressDisplayer* displayer);
  virtual ParameterSet defaultParameters() const;

  virtual void createMesh
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );
  
  virtual void createCase
  (
    OpenFOAMCase& cm,
    const ParameterSet& p
  );
};

}

#endif // INSIGHT_EXPLICITVORTEX_H
