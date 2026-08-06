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

#include "internalpressurelossbase.h"
#include "internalpressureloss_pdl.h"

namespace insight
{




class InternalPressureLoss
: public InternalPressureLossBase
{
public:
  static void modifyDefaults(ParameterSet& p);
#include INTERNALPRESSURELOSS_PDL_InternalPressureLoss
/*
PARAMETERSET>>>

inherits insight::InternalPressureLossBase::Parameters

addTo_makeDefault { modifyDefaults(p); }


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

    double pAmbient_;
    si::Temperature globalTmin, globalTmax;
  };

  addParameterMembers_SupplementedInputData(InternalPressureLoss::Parameters);


public:
    declareType("Internal Pressure Loss");

    InternalPressureLoss(
        const std::shared_ptr<supplementedInputDataBase>& sp );

    std::unique_ptr<OpenFOAMCaseElement> createWallBC(insight::OpenFOAMCase& cm, const std::string& patchName, const OFDictData::dict& boundaryDict) const override;
    double inletTemperature(const std::string& patchName) const override;

    std::unique_ptr<FVNumerics> numericsCaseElement(OpenFOAMCase& cm) const override;

    std::unique_ptr<calculateTotalPressure::Parameters>
    totalPressureCalculationParameters() const override;

    double ambientPressure() const override;
    std::unique_ptr<porousZoneOption::Parameters>
        porousZoneParameters(
            const std::string& label,
            const Parameters::geometry_default_type::role_porousVolume_type* vol) const override;

    void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
    ResultSetPtr evaluateResults(OpenFOAMCase& cmp, ProgressDisplayer& parentActionProgress) override;

    static std::string category() { return "Generic Analyses"; }
    static AnalysisDescription description()
    { return { "Internal Pressure Loss",
            "Determination of internal pressure loss by CFD a simulation"}; }
};






}

#endif // INSIGHT_INTERNALPRESSURELOSS_H
