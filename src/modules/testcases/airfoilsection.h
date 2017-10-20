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

#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamparameterstudy.h"
#include "base/stltools.h"

#include "base/boost_include.h"

namespace insight 
{


class AirfoilSection 
: public insight::OpenFOAMAnalysis
{
public:
#include "airfoilsection__AirfoilSection__Parameters.h"
/*
PARAMETERSET>>> AirfoilSection Parameters

geometry = set
{

 c         = double 1.0 "[m] chord length\n \\includegraphics[width=\\linewidth]{testcases/airfoilsection_sketches_L}"
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
 The file is simple text file with the 2D points on the foil contour. X-coordinate is in the first column and Y-coordinate in the seconed column. The X axis is aligned with the chord.
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

} "Parameters of the fluid"


<<<PARAMETERSET
*/  


  std::string in_, out_, up_, down_, fb_, foil_;
public:
  declareType("Airfoil 2D");
  
  AirfoilSection(const ParameterSet& ps, const boost::filesystem::path& exepath);
    
  static insight::ParameterSet defaultParameters();
  
  static std::string category() { return "Generic Analyses"; }

  virtual void createCase(insight::OpenFOAMCase& cm);
  virtual void createMesh(insight::OpenFOAMCase& cm);
  virtual insight::ResultSetPtr evaluateResults(insight::OpenFOAMCase& cm);
  
};


extern RangeParameterList rpl_AirfoilSectionPolar;

class AirfoilSectionPolar 
: public OpenFOAMParameterStudy<AirfoilSection, rpl_AirfoilSectionPolar>
{
public:
    declareType("Airfoil 2D Polar");
    
    static std::string category() { return "Generic Analyses"; }
    
    AirfoilSectionPolar(const ParameterSet& ps, const boost::filesystem::path& exepath);    
    virtual void evaluateCombinedResults(ResultSetPtr& results);
};

}

#endif // INSIGHT_AIRFOILSECTION_H
