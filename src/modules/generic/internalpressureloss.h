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

#include "base/exception.h"
#include "base/units.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamparameterstudy.h"

#include "internalpressureloss__InternalPressureLoss__Parameters_headers.h"

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


geometry =  labeledarray "geometry_%d" [ set {

    file = cadgeometry "" "Part of the geometry. May be an STL file or CAD exchange format (STEP or IGES)." *necessary

    lm = int -1 "Minimum refinement level. If value is negative, the global minimum refinement level is used."
    lx = int -1 "Maximum refinement level. If value is negative, the global maximum refinement level is used."

    role = selectablesubset {{

        refinementOnly set {
            mode = selection ( inside outside distance ) inside "Refinement mode"
            dist = double 1e15 "Maximum distance for refinement. Set very large, if mode is inside." *necessary
        }

        wall set {
            roughness_z0 = double 0 "Wall roughness height"
        }

        symmetry set {
        }

        inlet set {
            specification = selectablesubset {{
              vector set {
                velocity = vector (0 0 0) ""
              }
              massFlow set {
                dotm = double 0.001 "[kg/s] mass flow"
              }
              volumetricFlow set {
                Q = double 0.001 "[m^3/s] volume flow"
              }
              pressureInlet set {
                ambientPressure = double 0. ""
              }
            }} vector "type of velocity specification"
        }

        outlet set {
           pressure = double 0. "pressure difference to ambient pressure at the outlet"
        }

        porousVolume set {
            d = double 0. "[Pa/m^2] darcy contribution" *necessary
            f = double 0. "[kg/m^3] forchheimer contribution" *necessary
        }

    }} wall "Boundary role of the geometry"

} ] *1
"Pieces of geometry.
 All pieces together must completely resemble the perimeter of the internal channel
 (except for the porousVolume)."





geometryscale = double 1e-3     "scaling factor to scale geometry files to meters"



mesh=set
{
  size		= double	10 		"[mm] Cell size of template mesh." *necessary
  minLevel      = int           0               "Minimum refinement level on geometry."
  maxLevel      = int           2               "Maximum refinement level on geometry."
  nLayers       = int           3               "Number of prism layers"
  tlayer= double 0.5 "Layer thickness value"
  erlayer = double 1.3 "Expansion ratio of layers"
  relativeSizes = bool true "Whether tlayer specifies relative thickness (absolute thickness if set to false)"
  PiM           = vector        (0 0 0)         "Seed point inside flow domain." *necessary
} "Properties of the computational mesh"




operation=set
{
  timeTreatment = selectablesubset {{
    steady set {}
    unsteady set {
      endTime = double 1 "[s] end time of the simulation"
    }
  }} steady "How to treat the time"

  thermalTreatment = selectablesubset {{

   isothermal set {}

   solve set {

    includeBuoyancy = selectablesubset {{
     no set { }
     yes set {
      outletPressure = double 1e5 "[Pa] Pressure at the outlet"
      gravityDirection = vector (0 0 1) "Direction of the gravity acceleration (pointing towards center of earth)"
     }
    }} no "Whether to include buoyancy effects"

    initialInternalTemperature = double 300 "[K] Temperature in the domain at simulation start"

    BCs = labeledarray keysFrom "../../../geometry" [ selectablesubset {{
     adiabatic set {}
     fixedTemperature set {
      temperature = double 300 "[K] Fixed temperature of the wall or inlet"
     }
    }} adiabatic "" ] *0 ""

   }

  }} isothermal "control the energy transport"

} "Definition of the operation point under consideration"




fluid=set
{

  rho		= double 	998.0 		"[kg/m^3] Density of the fluid"
  nu		= double 	1e-6 		"[m^2/s] Viscosity of the fluid"
  turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"

} "Parameters of the fluid"



eval = set {
 averageFraction = double 0.2
"fraction of the the total simulation duration,
 over which the iteration history of each quantity is averaged to obtain the reported figure."

 additionalCutPlaneLocations = labeledarray "cutplane_%d" [
   vector (0 0 0) ""
 ] *0
} "Parameters for evaluation"


<<<PARAMETERSET
*/


  struct supplementedInputData
      : public supplementedInputDataDerived<Parameters>
  {
  public:
    supplementedInputData(
          ParameterSetInput ip,
          const boost::filesystem::path& workDir,
          ActionProgress& progress );

    BoundingBox bb_;
    arma::mat L_;

    int nx_, ny_, nz_;

    // cad::FeaturePtr inlet_, outlet_;
    // std::map<std::string, cad::FeaturePtr> walls_;

    boost::filesystem::path stldir_;
    std::string fn_inlet_, fn_outlet_;

    double pAmbient_;
    si::Temperature globalTmin, globalTmax;
  };

  addParameterMembers_SupplementedInputData(InternalPressureLoss::Parameters);


public:
    declareType("Internal Pressure Loss");

    InternalPressureLoss(
        const std::shared_ptr<supplementedInputDataBase>& sp );

    std::pair<std::set<std::string>, std::set<std::string> >
    findInOutPatches() const;

    void calcDerivedInputData(ProgressDisplayer& parentActionProgress) override;
    void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    void createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    void applyCustomPreprocessing(OpenFOAMCase& cm, ProgressDisplayer& progress) override;
    
    ResultSetPtr evaluateResults(OpenFOAMCase& cmp, ProgressDisplayer& parentActionProgress) override;

    static std::string category() { return "Generic Analyses"; }
    static AnalysisDescription description()
    { return { "Internal Pressure Loss",
            "Determination of internal pressure loss by CFD a simulation"}; }
};



extern RangeParameterList rpl_InternalPressureLossCharacteristics;


class InternalPressureLossCharacteristics
    : public OpenFOAMParameterStudy<InternalPressureLoss, rpl_InternalPressureLossCharacteristics>
{
public:
    declareType("Internal Pressure Loss Characteristic Map");

    InternalPressureLossCharacteristics(
        const std::shared_ptr<supplementedInputDataBase>& sp );

//    virtual void evaluateForceFits(PlotCurveList& crv) const;
    void evaluateCombinedResults(ResultSet& results) override;

    static AnalysisDescription description()
    { return { typeName,
                "Internal pressure loss calculation for multiple volume fluxes" }; }
};




}

#endif // INSIGHT_INTERNALPRESSURELOSS_H
