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

#ifndef INSIGHT_AIRFOILSECTION_H
#define INSIGHT_AIRFOILSECTION_H

#include "base/supplementedinputdata.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamparameterstudy.h"
//#include "base/stltools.h"


namespace insight 
{


class AirfoilSection 
: public insight::OpenFOAMAnalysis
{
public:
#include "airfoilsection__AirfoilSection__Parameters.h"
/*
PARAMETERSET>>> AirfoilSection Parameters

inherits OpenFOAMAnalysis::Parameters

geometry = set
{

 alpha     = double 0.0 "[deg] angle of attack\n \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_alpha}"
 
 LinByc    = double 1.0 "[-] Upstream extent of the channel.
 The length is normalized by the chord length.
 \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_L}"
 
 LoutByc   = double 4.0 "[-] Downstream extent of the channel
 The length is normalized by the chord length.
 \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_L}"
 
 HByc      = double 2.0 "[-] Height of the channel below and above the airfoil.
 The height is normalized by the chord length.
 \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_H}"
 
 foilfile  = path "foil.csv" "File with tabulated coordinates on the foil surface.

 If the extension is .dat, an XFLR compatible file is expected.

 Otherwise, the file is expected to be a simple text file with the 2D points on the foil contour. X-coordinate is in the first column and Y-coordinate in the seconed column. The X axis is aligned with the chord.
 The points need to be ordered but neither the direction along the contour nor the location of the starting point is important. The contour will be automatically closed so the first and last point must no coincide.
 \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_csv}"
 
} "Geometrical properties of the numerical tunnel"


mesh = set
{

 nc        = int 15 "# cells along span"
 lmfoil    = int 5 "minimum refinement level at foil surface\n \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_1}"
 lxfoil    = int 6 "maximum refinement level at foil surface\n \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_1}"
 nlayer    = int 10 "number of prism layers"

} "Properties of the computational mesh"


operation = set
{

 vinf      = double 30.8 "[m/s] inflow velocity"

} "Definition of the operation point under consideration"


fluid = set
{

 rho       = double 1.0 "[kg/m^3] Density of the fluid"
 nu        = double 1.5e-5 "[m^2/s] Viscosity of the fluid"
 turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"

} "Parameters of the fluid"

run=set
{
 residual       = double 1e-5
 "The required relative change in forces for considering the solution as converged.

Basis is the moving average of the forces over the last half of the acquired samples.
The maximum relative change between the last 15 average values needs to stay below this limit."

} "Execution parameters"

<<<PARAMETERSET
*/


  struct supplementedInputData
        : public supplementedInputDataDerived<Parameters>
  {
    supplementedInputData(
        ParameterSetInput ip,
        const boost::filesystem::path& workDir,
        ProgressDisplayer& progress = consoleProgressDisplayer
        );

    std::string in_, out_, up_, down_, fb_, foil_;
    arma::mat contour_;
    double c_;
  };

  addParameterMembers_SupplementedInputData(AirfoilSection::Parameters);


public:
  declareType("Airfoil 2D");
  
  AirfoilSection(
      const std::shared_ptr<supplementedInputDataBase>& sp );

  virtual void calcDerivedInputData(ProgressDisplayer& progress);

  virtual void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& progress);
  virtual void createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& progress);
  virtual insight::ResultSetPtr evaluateResults(insight::OpenFOAMCase& cm, ProgressDisplayer& progress);
  
  static std::string category() { return "Generic Analyses"; }
  static AnalysisDescription description() { return {"Airfoil 2D", "Steady RANS simulation of a 2-D flow over an airfoil section"}; }
};




extern RangeParameterList rpl_AirfoilSectionPolar;




class AirfoilSectionPolar 
: public OpenFOAMParameterStudy<AirfoilSection, rpl_AirfoilSectionPolar>
{
public:
    declareType("Airfoil 2D Polar");
    
    AirfoilSectionPolar(
        const std::shared_ptr<supplementedInputDataBase>& sp );

    virtual void evaluateCombinedResults(ResultSetPtr& results);

    static AnalysisDescription description()
    {
        return {
            "Polar of Airfoil",
            "Computes the polar of a 2D airfoil section using CFD" }; }
};

}

#endif // INSIGHT_AIRFOILSECTION_H
