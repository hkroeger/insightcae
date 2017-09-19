/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#ifndef INSIGHT_FLATPLATEBL_H
#define INSIGHT_FLATPLATEBL_H

#include "openfoam/openfoamanalysis.h"

namespace insight {

class FlatPlateBL 
: public OpenFOAMAnalysis
{
public:
  
#include "flatplatebl__FlatPlateBL__Parameters.h"
/*
PARAMETERSET>>> FlatPlateBL Parameters

inherits OpenFOAMAnalysis::Parameters

geometry=set
{
 HbyDelta99 = double 6.0 "Domain height above plate, divided by initial BL thickness at the inlet"
 WbyDelta99 = double 4.0 "Domain height above plate, divided by initial BL thickness at the inlet"
 LbyDelta99 = double 20.0 "Length of the domain, divided by initial BL thickness at the inlet"
} "Geometrical properties of the domain"

mesh=set
{
 dxplus0 = double 40 "dimensionless streamwise mesh spacing $\\delta x^+$ at the inlet"
 yplus0 = double 2 "dimensionless wall-normal mesh spacing at the inlet"
 dzplus0 = double 20 "dimensionless spanwise mesh spacing at the inlet"
 nh = int 64 "# cells in vertical direction"
 nl = int 20 "# number of near wall layers"
 layerratio = double 1.2 "near wall layer grading"
 twod = bool false "select method of transition enforcement"
} "Properties of the computational mesh"
    
operation=set
{
  Retheta0 = double 1200 "Momentum-thickness reynolds number at x=0."
  uinf = double 17.78 "[m/s] free-stream velocity"   
} "Definition of the operation point under consideration"

fluid = set 
{
  nu = double 1.8e-5 "[m^2/s] Viscosity of the fluid"
} "Parameters of the fluid"

run = set 
{
 filteredconvection = bool false "use filteredLinear instead of linear in LES"
 
 regime = selectablesubset
 {{
  
  steady
  set{
   iter = int 1000 "number of outer iterations after which the solver should stop"
  }
  
  unsteady
  set{
   inittime = double 2 "[T] length of grace period before averaging starts (as multiple of flow-through time)"
   meantime = double 10 "[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"
   mean2time = double 10 "[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"
  }
  
 }} steady "The simulation regime"

} "Solver parameters"

eval = set
{
 bc_extractsections 	= array [ set {
    name_prefix = string "extractsection" "name of the section"
    x = double 1 "location of the section"
  } ]*0 	"Sections, where BC profiles shall be extracted"
} "Parameters for evaluation after solver run"

<<<PARAMETERSET
*/
  
protected:
#ifndef SWIG
    const static std::vector<double> sec_locs_;
#endif
    
    /**
     * the momentum thickness reynolds number at the tripping location.
     * This is a parameter but can be overwritten by computeInitialLocation 
     * if e.g. velocity profiles are supplied in derived analysis classes
     */
    double Retheta0_;
    
    /**
     * the momentum thickness at initial location, determined in computeInitialLocation
     */
    double theta0_;

    /**
     * the 99% velocity BL thickness, determined in computeInitialLocation
     */
    double delta99_0_;
    
    /**
     * the friction coefficient at the initial location, determined in computeInitialLocation
     */
    double cf_0_;
    
    /**
     * friction velocity at the initial location, determined in computeInitialLocation
     */
    double utau_0_;
    
    /**
     * the Reynolds number with the (virtual) turbulent running length, determined in computeInitialLocation
     */
    double Rex_0_;
    
    double L_, H_, W_;
    
    /**
     * the Reynolds number with the (virtual) turbulent running length at the end of the domain
     */
    double Rex_e_;
    
    double delta99_e_, Re_theta2e_, uinf_, ypfac_ref_, deltaywall_ref_, y_final_, gradl_, gradh_, T_, dtrip_, gradax_;
    int nax_, nlat_;
    
    double avgStart_, avg2Start_, end_;
    
    std::string in_, /*out_, top_*/out_top_, cycl_prefix_, approach_, trip_;
    inline std::string tripMaster() const { return trip_+"_master"; }
    inline std::string tripSlave() const { return trip_+"_slave"; }
  
  /**
   * number of profiles for homogeneous averages
   */
  int n_hom_avg_=10;

public:
  declareType("Flat Plate Boundary Layer Test Case");
  
  FlatPlateBL(const ParameterSet& ps, const boost::filesystem::path& exepath);
  
  static ParameterSet defaultParameters();
  static std::string category() { return "Validation Cases"; }
  
  virtual void computeInitialLocation();
  virtual void calcDerivedInputData();
  
  virtual void createInflowBC(OpenFOAMCase& cm, const OFDictData::dict& boundaryDict) const;
  virtual void createCase(OpenFOAMCase& cm);
  virtual void createMesh(OpenFOAMCase& cm);

  virtual void evaluateAtSection
  (
    OpenFOAMCase& cm, 
    ResultSetPtr results, double x, int i,
    const Interpolator& cf,
    const std::string& UMeanName,
    const std::string& RFieldName,
    const FlatPlateBL::Parameters::eval_type::bc_extractsections_default_type* extract_section=NULL
  );  
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
  
  virtual Analysis* clone();
  
  /**
   * solves the function G(Alpha,D) numerically
   */
  static double G(double Alpha, double D);
  
  /**
   * computes the friction coefficient of a flat plate (total frictional resistance)
   * @Re Reynolds number formulated with length of plate
   */
  static double cw(double Re, double Cplus=5.0);

  /**
   * computes the friction coefficient of a flat plate at station x
   * @Rex Reynolds number formulated with running distance x
   */
  enum cf_method {cf_method_Cengel, cf_method_Schlichting};

  static double cf(double Rex, double Cplus=5.0, cf_method=cf_method_Cengel);
  
  /**
   * computes the Reynolds number with BL layer thickness delta99 Redelta99=uinf*delta99/nu at axial station x = Rex*nu/Uinf
   */
  enum Redelta99_method {Redelta99_method_Cengel, Redelta99_method_Schlichting};
  
  static double Redelta99(double Rex, Redelta99_method=Redelta99_method_Cengel);
  
  /**
   * computes the Reynolds number with BL momentum thickness Redelta2=uinf*delta2/nu delta2 at axial station x = Rex*nu/Uinf
   */
  enum Redelta2_method {Redelta2_method_Cengel, Redelta2_method_Schlichting};
  
  static double Redelta2(double Rex, Redelta2_method method=Redelta2_method_Cengel);
  static double Rex(double Redelta2, Redelta2_method method=Redelta2_method_Cengel);
  
  static arma::mat integrateDelta123(const arma::mat& uByUinf_vs_y);
  static double searchDelta99(const arma::mat& uByUinf_vs_y);
};

}

#endif // INSIGHT_FLATPLATEBL_H
