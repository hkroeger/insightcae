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




// class BoundaryProperties
// {
// public:
// #include "internalpressureloss__BoundaryProperties__Parameters.h"
// /*
// PARAMETERSET>>> BoundaryProperties Parameters

// role = selectablesubset {{

//  wall set {

//     wallBC = selectablesubset {{

//      adiabatic set {}

//      fixedTemperature set {
//       wallTemperature = double 300 "[K] Fixed temperature of the walls"
//      }

//     }} adiabatic ""

// <<<PARAMETERSET
// */

// private:
//     std::string patchName_;
//     Parameters p_;

// public:
//     InternalWallProperties(
//         const ParameterSet& ps
//         );

//     void insertBC(insight::OpenFOAMCase& cm, OFDictData::dict& boundaryDict);
// };




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


geometry = set {
      walls = labeledarray "wall_%d" [ set {
        file = path "" 	"Part of the geometry, excluding in- and outlet. May be an STL file or CAD exchange format (STEP or IGES)." *necessary
      } ] *1 "Pieces of geometry. All pieces together must completely resemble the perimeter of the internal channel."
      inlet = path "" "Triangulated geometry of inlet alone. May be an STL file or CAD exchange format (STEP or IGES)." *necessary
      outlet = path "" "Triangulated geometry of outlet alone. May be an STL file or CAD exchange format (STEP or IGES)." *necessary
} "Specification of geometry"


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
  Q		= double 	0.001 		"[m^3/s] volumetric flux into inlet" *necessary

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

    inletTemperature = double 300 "[K] Temperature at the inlet"

    initialInternalTemperature = double 300 "[K] Temperature in the domain at simulation start"

    wallBCs = labeledarray keysFrom "../../../geometry/walls" [ selectablesubset {{
     adiabatic set {}
     fixedTemperature set {
      wallTemperature = double 300 "[K] Fixed temperature of the walls"
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

    cad::FeaturePtr inlet_, outlet_;
    std::map<std::string, cad::FeaturePtr> walls_;

    boost::filesystem::path stldir_;
    std::string fn_inlet_, fn_outlet_;

    double pAmbient_;
    si::Temperature globalTmin, globalTmax;
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
    
    ResultSetPtr evaluateResults(OpenFOAMCase& cmp, ProgressDisplayer& parentActionProgress) override;
};



extern RangeParameterList rpl_InternalPressureLossCharacteristics;


class InternalPressureLossCharacteristics
    : public OpenFOAMParameterStudy<InternalPressureLoss, rpl_InternalPressureLossCharacteristics>
{
protected:
//    std::map<std::string, std::map<std::string, std::shared_ptr<CoefficientFit> > > fits_;
    //  boost::ptr_vector<CoefficientFit> fits_;

public:
    declareType("Internal Pressure Loss Characteristic Map");

    InternalPressureLossCharacteristics
        (
            const ParameterSet& ps,
            const boost::filesystem::path& exepath,
            ProgressDisplayer& pd = consoleProgressDisplayer
            );

    static std::string category() { return "Generic Analyses"; }

//    virtual void evaluateForceFits(PlotCurveList& crv) const;
    void evaluateCombinedResults(ResultSetPtr& results) override;
};




}

#endif // INSIGHT_INTERNALPRESSURELOSS_H
