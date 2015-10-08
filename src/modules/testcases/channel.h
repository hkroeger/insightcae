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
#include "channel__ChannelBase__Parameters.h"
/*
PARAMETERSET>>> ChannelBase Parameters

inherits OpenFOAMAnalysis::Parameters

geometry=set
{
 H = double 2.0 "[m] Height of the channel"
 B = double 4.19 "[m] Width of the channel"
 L = double 12.56 "[m] Length of the channel"
} "Geometrical properties of the domain"

mesh=set
{
 nh = int 64 "# cells in vertical direction"
 fixbuf = bool false "fix cell layer size inside buffer layer"
 nl = int 15 "number of near wall layers"
 layerratio = double 1.1 "near wall layer grading"
 dzplus = double 15 "Dimensionless grid spacing in spanwise direction"
 dxplus = double 60 "Dimensionless grid spacing in axial direction"
 ypluswall = double 2 "yPlus at the wall grid layer"
 twod = bool false "Whether to create a two-dimensional case"
} "Properties of the computational mesh"
    
operation=set
{
 Re_tau = double 180 "[-] Friction-Velocity-Reynolds number"
} "Definition of the operation point under consideration"

fluid = set 
{
  nu = double 1.8e-5 "[m^2/s] Viscosity of the fluid"
} "Parameters of the fluid"

run = set 
{
 filteredconvection = bool false "use filteredLinear instead of linear in LES"
 eval2 = bool true "Whether to evaluate second order statistics"
 
 regime = selectablesubset
 {{
  
  steady
  set{
   iter = int 1000 "number of outer iterations after which the solver should stop"
  }
  
  unsteady
  set{
   inittime = double 10 "[T] length of grace period before averaging starts (as multiple of flow-through time)"
   meantime = double 10 "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"
   mean2time = double 10 "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"
  }
  
 }} steady "The simulation regime"

} "Solver parameters"

<<<PARAMETERSET
*/

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
  
  
  /**
   * number of profiles for homogeneous averages
   */
  const int n_hom_avg=10;

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
  
  double gradl_;
  
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

  double avgStart_, avg2Start_, end_;  
  
  /**
   * stored after check in evaluateResults of base class
   */
  std::string UMeanName_, RFieldName_;
  
public:
  declareType("Channel Flow Test Case");
  
  ChannelBase(const NoParameters& p = NoParameters());
  ~ChannelBase();
  
  virtual ParameterSet defaultParameters() const;
  
  std::string cyclPrefix() const;
  virtual void calcDerivedInputData();

  virtual void createMesh
  (
    OpenFOAMCase& cm
  );
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
  
  virtual void evaluateAtSection(
    OpenFOAMCase& cm,
    ResultSetPtr results, double x, int i
  );
    
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cmp);
  
};




class ChannelCyclic
: public ChannelBase
{
public:
  declareType("Channel Flow Test Case (Axial Cyclic)");
  
  ChannelCyclic(const NoParameters& p = NoParameters() );
  
  virtual ParameterSet defaultParameters() const;

  virtual void createMesh
  (
    OpenFOAMCase& cm
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
  
};


}

#endif // INSIGHT_CHANNEL_H
