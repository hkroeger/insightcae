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
public:
  /**
   * convert friction velocity Reynolds number into bulk velocity Re
   */
  static double Re(double Retau);

  /**
   * convert friction velocity Reynolds number into bulk velocity Re
   */
  static double Retau(double Re);

  /**
   * compute factor Umax/Ubulk
   */
  static double UmaxByUbulk(double Retau);

protected:
  std::string cycl_in_, cycl_out_;
  
  // Derived input data
  /**
   * Bulk velocity reynolds number
   */
  double Re_;
  
  /**
   * friction velocity
   */
  double utau_;
  
  /**
   * bulk velocity
   */
  double Ubulk_;
  
  /** 
   * flow-through time
   */
  double T_;
  
  /**
   * viscosity
   */
  double nu_;
  
  /**
   * height of cell layer nearest to wall
   */
  double ywall_;
  
  /**
   * number of cells along flow direction
   */
  int nax_;
  
  /**
   * number of cells along span
   */
  int nb_;
  
  /**
   * number of cells along half height
   */
  int nh_;
  
  /**
   * number of cells along half height
   */
  int nhbuf_;
  
  /**
   * grading towards wall
   */
  double gradh_;
  
  /**
   * height of buffer layer
   */
  double hbuf_;
  
public:
#ifndef SWIG
  declareType("Channel Flow Test Case");
#endif
  
  ChannelBase(const NoParameters& = NoParameters());
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

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  
  virtual void evaluateAtSection(
    OpenFOAMCase& cm, const ParameterSet& p, 
    ResultSetPtr results, double x, int i
  );
    
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);
  
};




class ChannelCyclic
: public ChannelBase
{
public:
#ifndef SWIG
  declareType("Channel Flow Test Case (Axial Cyclic)");
#endif
  
  ChannelCyclic(const NoParameters&);
  
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

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};

class ChannelInflow
: public ChannelBase
{
  
#ifndef SWIG
  const static int ntpc_ = 4;
  const static char* tpc_names_[ntpc_]; 
  const static double tpc_xlocs_[ntpc_];
#endif
  
public:
#ifndef SWIG
  declareType("Channel Flow Test Case (Inflow Generator)");
#endif
  
  ChannelInflow(const NoParameters&);
  
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

  ResultSetPtr evaluateResults(OpenFOAMCase& cm, const ParameterSet& p);

  virtual void applyCustomOptions(OpenFOAMCase& cm, const ParameterSet& p, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm, const ParameterSet& p);
  
};

}

#endif // INSIGHT_CHANNEL_H
