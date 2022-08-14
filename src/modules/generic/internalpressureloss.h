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
#ifndef INSIGHT_INTERNALPRESSURELOSS_H
#define INSIGHT_INTERNALPRESSURELOSS_H

#include "openfoam/openfoamanalysis.h"
#include "cadparametersetvisualizer.h"
#include "openfoam/openfoamtools.h"

namespace insight
{
  
class InternalPressureLoss
: public insight::OpenFOAMAnalysis
{
public:
  static void modifyDefaults(ParameterSet& p);
#include "internalpressureloss__InternalPressureLoss__Parameters.h"
/*
PARAMETERSET>>> InternalPressureLoss Parameters

inherits OpenFOAMAnalysis::Parameters

addTo_makeDefault { modifyDefaults(p); }


geometry = selectablesubset {{

 STEP set {
      cadmodel          = path 		"cadmodel.stp" 		"CAD model file." *necessary

      inout = selectablesubset {{

       named_surfaces set {
         inlet_name	= string	"INLET"			"Name of inlet surface in CAD model." *necessary
         outlet_name	= string	"OUTLET"		"Name of outlet surface in CAD model." *necessary
       }

       extra_files set {
         inlet_model	= path          "inlet.stp"		"File with inlet surface." *necessary
         outlet_model	= path          "outlet.stp"		"File with outlet surface." *necessary
       }

      }} named_surfaces "Specification of inlet and outlet surfaces"
 }

 STL set {
      cadmodel          = path 		"cadmodel.stlb" 	"Triangulated geometry, excluding in- and outlet." *necessary
      inlet             = path          "inlet.stlb"		"Triangulated geometry of inlet alone." *necessary
      outlet    	= path          "outlet.stlb"		"Triangulated geometry of outlet alone." *necessary
 }

}} STL "Specification of geometry"

geometryscale = double 1e-3     "scaling factor to scale geometry files to meters"

mesh=set
{
  size		= double	10 		"[mm] Cell size of template mesh." *necessary
  minLevel      = int           0               "Minimum refinement level on geometry."
  maxLevel      = int           2               "Maximum refinement level on geometry."
  nLayers       = int           3               "Number of prism layers"
  PiM           = vector        (0 0 0)         "Seed point inside flow domain." *necessary
} "Properties of the computational mesh"

operation=set
{
  Q		= double 	0.001 		"[m^3/s] volumetric flux into inlet" *necessary
} "Definition of the operation point under consideration"

fluid=set
{

  rho		= double 	998.0 		"[kg/m^3] Density of the fluid"
  nu		= double 	1e-6 		"[m^2/s] Viscosity of the fluid"
  turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"

} "Parameters of the fluid"

<<<PARAMETERSET
*/


  struct supplementedInputData
      : public supplementedInputDataDerived<Parameters>
  {
  public:
    supplementedInputData(std::unique_ptr<Parameters> p,
                          const boost::filesystem::path& workDir,
                          ProgressDisplayer& progress = consoleProgressDisplayer );

    BoundingBox bb_;
    arma::mat L_;

    int nx_, ny_, nz_;

    boost::filesystem::path wallstlfile_, inletstlfile_, outletstlfile_;

  };

#ifndef SWIG
  defineBaseClassWithSupplementedInputData(Parameters, supplementedInputData)
#endif


public:
    declareType("Internal Pressure Loss");

    InternalPressureLoss(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& pd);

    static std::string category() { return "Generic Analyses"; }
    
    void calcDerivedInputData(ProgressDisplayer& parentActionProgress) override;
    void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    void createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    
    virtual ResultSetPtr evaluateResults(OpenFOAMCase& cmp, ProgressDisplayer& parentActionProgress);
};




class InternalPressureLoss_ParameterSet_Visualizer
 : public CADParameterSetVisualizer
{
public:
    typedef InternalPressureLoss::Parameters Parameters;

public:
    void recreateVisualizationElements() override;
};

}

#endif // INSIGHT_INTERNALPRESSURELOSS_H
