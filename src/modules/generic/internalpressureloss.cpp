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
#include "internalpressureloss.h"

#include "base/cppextensions.h"
#include "base/exception.h"
#include "base/factory.h"
#include "base/units.h"
#include "boost/algorithm/string/case_conv.hpp"
#include "boost/algorithm/string/join.hpp"
#include "boost/format/format_fwd.hpp"
#include "boost/iterator_adaptors.hpp"
#include "cadfeature.h"
#include "cadgeometryparameter.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
#include "openfoam/caseelements/boundaryconditions/exptdatainletbc.h"
#include "openfoam/caseelements/boundaryconditions/suctioninletbc.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"

#include "openfoam/caseelements/basic/limitquantities.h"
#include "openfoam/caseelements/evaluation/calculatetotalpressure.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"
#include "openfoam/snappyhexmesh.h"

#include "base/vtktransformation.h"

#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/unsteadyincompressiblenumerics.h"
#include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
#include "openfoam/caseelements/basic/passivescalar.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/basic/gravity.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"
#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
#include "openfoam/caseelements/boundaryconditions/massflowbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/analysiscaseelements.h"
#include "openfoam/caseelements/thermodynamics/compressiblesinglephasethermophysicalproperties.h"

#include "cadfeatures.h"
#include "cadfeatures/stl.h"
#include "datum.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <tuple>

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight 
{


defineType(InternalPressureLoss);
Analysis::Add<InternalPressureLoss> addInternalPressureLoss;


void InternalPressureLoss::modifyDefaults(ParameterSet& p)
{
    p.setBool("run/potentialinit", true);
}

InternalPressureLoss::InternalPressureLoss(
    const std::shared_ptr<supplementedInputDataBase> &sp)
    : InternalPressureLossBase(sp)
{}







std::unique_ptr<OpenFOAMCaseElement>
InternalPressureLoss::createWallBC(
    OpenFOAMCase &cm, const std::string &patchName, const OFDictData::dict &boundaryDict) const
{
    WallBC::Parameters wp;

    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {
        auto &tbcc=thermsolve->BCs.at(patchName);
        if (auto* tbc=boost::get<Parameters::operation_type::thermalTreatment_solve_type
                                   ::BCs_default_adiabatic_type>(
                &tbcc))
        {
            wp.set_heattransfer(
                std::make_shared<HeatBC::AdiabaticBC>()
                );
        }
        else if (auto* tbc=boost::get<Parameters::operation_type::thermalTreatment_solve_type
                                        ::BCs_default_fixedTemperature_type>(
                     &tbcc))
        {
            wp.set_heattransfer(
                std::make_shared<HeatBC::FixedTemperatureBC>(
                    HeatBC::FixedTemperatureBC::Parameters()
                        .set_T( FieldData::uniformSteady(vec1(tbc->temperature)) )
                    ));
        }
        else
            throw insight::UnhandledSelection();
    }

    //else adiabtic (is default of WallBC)
    return std::make_unique<WallBC>(cm, patchName, boundaryDict, wp);
}




double InternalPressureLoss::inletTemperature(const std::string &patchName) const
{
    double T=300.;
    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {
        auto &tbcc=thermsolve->BCs.at(patchName);
        if (auto* tbc=boost::get<Parameters::operation_type::thermalTreatment_solve_type
                                   ::BCs_default_adiabatic_type>(
                &tbcc))
        {
            throw insight::Exception("invalid temperature BC at boundary %s: fixed temperature required",
                                     patchName.c_str());
        }
        else if (auto* tbc=boost::get<Parameters::operation_type::thermalTreatment_solve_type
                                        ::BCs_default_fixedTemperature_type>(
                     &tbcc))
        {
            T=tbc->temperature;
        }
        else
            throw insight::UnhandledSelection();
    }
    return T;
}

std::unique_ptr<FVNumerics> InternalPressureLoss::numericsCaseElement(OpenFOAMCase &cm) const
{
    if (const auto* iso =
        boost::get<Parameters::operation_type::thermalTreatment_isothermal_type>(
            &p().operation.thermalTreatment))
    {
        if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                &p().operation.timeTreatment))
        {
            return std::make_unique<steadyIncompressibleNumerics>(cm);
        }
        else if (const auto* unsteady =
                 boost::get<Parameters::operation_type::timeTreatment_unsteady_type>(
                     &p().operation.timeTreatment))
        {
            unsteadyIncompressibleNumerics::Parameters uinp;
            uinp.set_time_integration(
                PIMPLESettings::Parameters().set_timestep_control(
                    PIMPLESettings::Parameters::timestep_control_adjust_type{10., 1.}
                    )
                );
            uinp.set_endTime(unsteady->endTime);
            uinp.set_deltaT(1e-3); // initial
            return std::make_unique<unsteadyIncompressibleNumerics>(cm, uinp);
        }
    }

    else if (const auto* thermsolve =
             boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
                 &p().operation.thermalTreatment))
    {

        bool buoyancy=false;
        if (const auto *buoy =
            boost::get<Parameters::operation_type::thermalTreatment_solve_type::includeBuoyancy_yes_type>(
                &thermsolve->includeBuoyancy))
        {
            buoyancy=true;
            gravity::Parameters gp;
            gp.g=normalized(buoy->gravityDirection)*9.81;
            cm.insert(new gravity(cm, gp));
        }
        else
        {
            PassiveScalar::Parameters psp;
            psp.fieldname="T";
            psp.internal=thermsolve->initialInternalTemperature;
            if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                    &p().operation.timeTreatment))
            {
                psp.underrelax=0.7;
            }
            cm.insert(new PassiveScalar(cm, psp));
        }

        if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                &p().operation.timeTreatment))
        {
            if (buoyancy)
            {
                buoyantSimpleFoamNumerics::Parameters bsfnp;
                bsfnp.pinternal=sp().pAmbient_;
                bsfnp.Tinternal=thermsolve->initialInternalTemperature;
                return std::make_unique<buoyantSimpleFoamNumerics>(cm, bsfnp);
            }
            else
            {
                return std::make_unique<steadyIncompressibleNumerics>(cm);
            }
        }
        else if (const auto* unsteady = boost::get<Parameters::operation_type::timeTreatment_unsteady_type>(
                     &p().operation.timeTreatment) )
        {
            if (buoyancy)
            {
                buoyantPimpleFoamNumerics::Parameters bpfnp;
                bpfnp.pinternal=sp().pAmbient_;
                bpfnp.Tinternal=thermsolve->initialInternalTemperature;
                CompressiblePIMPLESettings::Parameters tip;
                tip.set_timestep_control(
                    PIMPLESettings::Parameters::timestep_control_adjust_type{10., 1.}
                    );
                bpfnp.set_time_integration(tip);
                bpfnp.set_endTime(unsteady->endTime);
                bpfnp.set_deltaT(1e-3); // initial
                return std::make_unique<buoyantPimpleFoamNumerics>(cm, bpfnp);
            }
            else
            {
                unsteadyIncompressibleNumerics::Parameters uinp;
                uinp.set_time_integration(
                    PIMPLESettings::Parameters().set_timestep_control(
                        PIMPLESettings::Parameters::timestep_control_adjust_type{10., 1.}
                        )
                    );
                uinp.set_endTime(unsteady->endTime);
                uinp.set_deltaT(1e-3); // initial
                return std::make_unique<unsteadyIncompressibleNumerics>(cm, uinp);
            }
        }

    }
    else throw insight::UnhandledSelection("thermal solution option");

    return nullptr;
}

std::unique_ptr<calculateTotalPressure::Parameters>
InternalPressureLoss::totalPressureCalculationParameters() const
{
    auto ctp=InternalPressureLossBase
        ::totalPressureCalculationParameters();

    OpenFOAMCase cm(OFEs::get(p().OpenFOAMAnalysis::Parameters::run.OFEname));
    auto num = numericsCaseElement(cm);

    if (num->isCompressible())
        ctp->set_rho(calculateTotalPressure::Parameters::rho_field_type
                    {"rho"}
                    );
    else
        ctp->set_rho(calculateTotalPressure::Parameters::rho_rhoInf_type
                    {p().fluid.rho}
                    );

    return ctp;
}

InternalPressureLossBase::SolverProperties InternalPressureLoss::solverProperties() const
{
    OpenFOAMCase cm(OFEs::get(p().OpenFOAMAnalysis::Parameters::run.OFEname));
    auto num = numericsCaseElement(cm);

    if (!num->isCompressible())
    {
        return {SolverProperties::Volume, p().fluid.rho,
                SolverProperties::Kinematic, 1./p().fluid.rho};
    }
    else
    {
        return {SolverProperties::Mass, 1.,
                SolverProperties::NonKinematic, 1.};
    }
}

double InternalPressureLoss::ambientPressure() const
{
    return sp().pAmbient_;
}

std::unique_ptr<porousZoneOption::Parameters>
InternalPressureLoss::porousZoneParameters(
    const std::string& label,
    const Parameters::geometry_default_type::role_porousVolume_type* vol
) const
{
    auto pzp=std::make_unique<porousZoneOption::Parameters>();
    pzp->set_name(label);
    pzp->porousZone.d=vec3(1,1,1)*vol->d/p().fluid.nu;
    pzp->porousZone.f=vec3(1,1,1)*2.*vol->f/p().fluid.rho;
    return pzp;
}




void InternalPressureLoss::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& pd)
{
    InternalPressureLossBase::createCase(cm, pd);

    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);

    auto& num = cm.findUniqueElement<FVNumerics>();

    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {

        bool buoyancy=false;
        if (const auto *buoy =
            boost::get<Parameters::operation_type::thermalTreatment_solve_type::includeBuoyancy_yes_type>(
                &thermsolve->includeBuoyancy))
        {
            buoyancy=true;
            gravity::Parameters gp;
            gp.g=normalized(buoy->gravityDirection)*9.81;
            cm.insert(new gravity(cm, gp));
        }
        else
        {
            PassiveScalar::Parameters psp;
            psp.fieldname="T";
            psp.internal=thermsolve->initialInternalTemperature;
            if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                    &p().operation.timeTreatment))
            {
                psp.underrelax=0.7;
            }
            cm.insert(new PassiveScalar(cm, psp));
        }
    }

    if (num.isCompressible())
    {
        compressibleSinglePhaseThermophysicalProperties::Parameters fp;
        cm.insert(new compressibleSinglePhaseThermophysicalProperties(cm, fp));
    }
    else
    {
        singlePhaseTransportProperties::Parameters spp;
        cm.insert(new singlePhaseTransportProperties(cm, spp));
    }

    if (!boost::filesystem::exists(executionPath()/"system"/"controlDict"))
    {
        // ensure there is a controlDict for patchArea later
        cm.createOnDisk(executionPath());
    }




    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {
        cm.insert(new surfaceIntegrate(cm, surfaceIntegrate::Parameters()
               .set_domain( surfaceIntegrate::Parameters::domain_patch_type( "outlet" ) )
               .set_fields( { "T" } )
               .set_operation( surfaceIntegrate::Parameters::areaAverage )
               .set_outputControl("timeStep")
               .set_outputInterval(1)
               .set_name("outlet_temperature")
       ));
    }

    cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters());

    if (num.isCompressible())
    {
        limitQuantities::Parameters lp;

        // limitQuantities::Parameters::limitVelocity_limit_type lpU;
        // lpU.max=100.;
        // lp.limitVelocity=lpU;

        limitQuantities::Parameters::limitTemperature_limit_type lpT;
        lpT.min=toValue(sp().globalTmin, si::degK);
        lpT.max=toValue(sp().globalTmax, si::degK);
        lp.limitTemperature=lpT;

        cm.insert(new limitQuantities(cm, lp));
    }
    
    insertTurbulenceModel(cm, p().fluid.turbulenceModel);
}



}
