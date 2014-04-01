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

#ifndef INSIGHT_CHANNEL_H
#define INSIGHT_CHANNEL_H

#include "base/linearalgebra.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamcaseelements.h"

namespace insight {

class ChannelBase 
: public OpenFOAMAnalysis
{
protected:
  std::string cycl_in_, cycl_out_;
  
  // Derived input data
  double Re_, Ubulk_, T_, nu_, ywall_;
  int nb_, nh_;
  double gradh_;
  
public:
  declareType("Channel Flow Test Case");
  
  ChannelBase(const NoParameters&);
  ~ChannelBase();
  
  virtual ParameterSet defaultParameters() const;
  
  std::string cyclPrefix() const;
  virtual void calcDerivedInputData(const ParameterSet& p);

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

  
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);
  
};




class ChannelCyclic
: public ChannelBase
{
public:
  declareType("Channel Flow Test Case (Axial Cyclic)");
  
  ChannelCyclic(const NoParameters&);
  
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

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};


}

#endif // INSIGHT_CHANNEL_H
