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

#ifndef INSIGHT_PIPE_H
#define INSIGHT_PIPE_H

#include "base/linearalgebra.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/blockmesh.h"

namespace insight 
{


  

class PipeBase 
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
  
  
public:
#include "pipe__PipeBase__Parameters.h"
    
/*
PARAMETERSET>>> PipeBase Parameters

geometry = set {
 D = double 2.0 "[m] Diameter of the pipe"
 L = double 12.0 "[m] Length of the pipe"
} "Geometrical properties of the bearing"
      
mesh = set {
 x = double 0.33 "Edge length of core block as fraction of diameter"
 fixbuf = bool false "fix cell layer size inside buffer layer"
 dzplus = double 15 "Dimensionless grid spacing in spanwise direction"
 dxplus = double 60 "Dimensionless grid spacing in axial direction"
 ypluswall = double 0.5 "yPlus at the wall grid layer"
} "Properties of the computational mesh"
      
operation = set {
 Re_tau = double 180 "[-] Friction-Velocity-Reynolds number"
} "Definition of the operation point under consideration"
      
run = set {
 perturbU = bool true "Whether to impose artifical perturbations on the initial velocity field"
} "Execution parameters"

evaluation = set {
 inittime = double 10 "[T] length of grace period before averaging starts (as multiple of flow-through time)"
 meantime = double 10 "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"
 mean2time = double 10 "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"
} "Options for statistical evaluation"

<<<PARAMETERSET
*/
  
protected:
  std::string cycl_in_, cycl_out_;
  double Lc_, rbuf_;
  int nc_, nr_, ncir_, nax_, nrbuf_;
  double nu_, gradr_, ywall_, Re_, Ubulk_, T_, utau_;
  
public:
  declareType("Pipe Flow Test Case");
  
  PipeBase(const ParameterSet& ps, const boost::filesystem::path& exepath);
  ~PipeBase();
  
  static ParameterSet defaultParameters();
  static std::string category() { return "Validation Cases"; }
  
  std::string cyclPrefix() const;
  virtual void calcDerivedInputData();
//   virtual double calcLc(const ParameterSet& p) const;
//   virtual int calcnc(const ParameterSet& p) const;
//   virtual int calcnr(const ParameterSet& p) const;
//   virtual double calcgradr(const ParameterSet& p) const;
//   virtual double calcywall(const ParameterSet& p) const;
//   virtual double calcRe(const ParameterSet& p) const;
//   virtual double calcUbulk(const ParameterSet& p) const;
//   virtual double calcT(const ParameterSet& p) const;
//   virtual double calcUtau(const ParameterSet& p) const;

  void insertBlocksAndPatches
  (
    OpenFOAMCase& cm,
    std::auto_ptr<insight::bmd::blockMesh>& bmd,
    const std::string& prefix = "",
    double xshift = 0.
  ) const;
  
  virtual void createMesh
  (
    OpenFOAMCase& cm
  );
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  virtual void evaluateAtSection(
    OpenFOAMCase& cm, 
    ResultSetPtr results, double x, int i
  );

  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
  
};




class PipeCyclic
: public PipeBase
{
public:
  declareType("Pipe Flow Test Case (Axial Cyclic)");
  
  PipeCyclic(const ParameterSet& ps, const boost::filesystem::path& exepath);
  
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




class PipeInflow
: public PipeBase
{
  
#ifndef SWIG
  const static int ntpc_ = 4;
  const static char* tpc_names_[ntpc_]; 
  const static double tpc_xlocs_[ntpc_];
#endif
  
public:
  declareType("Pipe Flow Test Case (Inflow Generator)");
  
  PipeInflow(const ParameterSet& ps, const boost::filesystem::path& exepath);
  
  static ParameterSet defaultParameters();
  static std::string category() { return "Validation Cases/Inflow Generator"; }
  
  virtual void createMesh
  (
    OpenFOAMCase& cm
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  ResultSetPtr evaluateResults(OpenFOAMCase& cm);

  virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
  
};




}

#endif // INSIGHT_PIPE_H
