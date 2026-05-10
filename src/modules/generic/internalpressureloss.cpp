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
#include "openfoam/caseelements/evaluation/calculatetotalpressure.h"
#include "openfoam/caseelements/basic/porouszone.h"
#include "openfoam/caseelements/basic/limitquantities.h"
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
    // auto& walls = p.get<LabeledArrayParameter>("geometry/walls");
    // auto& wallconf = p.get<LabeledArrayParameter>("operation/thermalTreatment/<solve>/wallBCs");
    // walls.newItemAdded.connect(
    //     [&walls,&wallconf](const std::string& label, ParameterPtr)
    //     {
    //         wallconf.getOrInsertDefaultValue(label);
    //     });
}




InternalPressureLoss::InternalPressureLoss(
    const std::shared_ptr<supplementedInputDataBase>& sp )
: OpenFOAMAnalysis(sp)
{}




void InternalPressureLoss::calcDerivedInputData(ProgressDisplayer& /*prg*/)
{
  reportIntermediateParameter("Lx", sp().L_(0), "model size in x direction", "m");
  reportIntermediateParameter("Ly", sp().L_(1), "model size in y direction", "m");
  reportIntermediateParameter("Lz", sp().L_(2), "model size in z direction", "m");
  reportIntermediateParameter("nx", sp().nx_, "initial grid cell numbers in direction x");
  reportIntermediateParameter("ny", sp().ny_, "initial grid cell numbers in direction y");
  reportIntermediateParameter("nz", sp().nz_, "initial grid cell numbers in direction z");

}




void InternalPressureLoss::createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& pp)
{
  cm.insert(new MeshingNumerics(cm, MeshingNumerics::Parameters()
                                        .set_np(np())
                                ));
    cm.createOnDisk(executionPath());
    
    using namespace insight::bmd;
    std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
    bmd->setScaleFactor(p().geometryscale);
    bmd->setDefaultPatch("walls", "wall");

    double eps=0.01*arma::min(sp().bb_.col(1)-sp().bb_.col(0));
    std::map<int, bmd::Point> pt = boost::assign::map_list_of
          (0, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,0)-eps, sp().bb_(2,0)-eps))
          (1, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,0)-eps, sp().bb_(2,0)-eps))
          (2, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,1)+eps, sp().bb_(2,0)-eps))
          (3, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,1)+eps, sp().bb_(2,0)-eps))
          (4, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,0)-eps, sp().bb_(2,1)+eps))
          (5, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,0)-eps, sp().bb_(2,1)+eps))
          (6, 	vec3(sp().bb_(0,1)+eps, sp().bb_(1,1)+eps, sp().bb_(2,1)+eps))
          (7, 	vec3(sp().bb_(0,0)-eps, sp().bb_(1,1)+eps, sp().bb_(2,1)+eps))
          .convert_to_container<std::map<int, bmd::Point> >()
          ;

    // create patches
    {
        bmd->addBlock
            (
                new Block(
                    {
                        pt[0], pt[1], pt[2], pt[3],
                        pt[4], pt[5], pt[6], pt[7]
                    },
                    sp().nx_, sp().ny_, sp().nz_
                    )
                );
    }
    int nb=bmd->nBlocks();
    cm.insert(bmd.release());
    cm.createOnDisk(executionPath());
    cm.runBlockMesh(executionPath(), nb, &pp);



    create_directory(sp().stldir_);

    snappyHexMeshConfiguration::Parameters shm_cfg;

    for (const auto&w: p().geometry)
    {
        int minLevel = w.second.lm>=0?w.second.lm:p().mesh.minLevel;
        int maxLevel = w.second.lx>=0?w.second.lx:p().mesh.maxLevel;

        if (auto *ref = boost::get<Parameters::geometry_default_type::role_refinementOnly_type>(
                &w.second.role))
        {
            shm_cfg.features.push_back(
                std::make_shared<snappyHexMeshFeats::RefinementGeometry>(
                    snappyHexMeshFeats::RefinementGeometry::Parameters()
                        .set_scalefactor(p().geometryscale)
                        .set_geometry(w.second.file)
                        .set_dist(ref->dist)
                        .set_mode(
                            static_cast<snappyHexMeshFeats::RefinementGeometry::Parameters::mode_type>(
                                ref->mode.value))
                        .set_level(maxLevel)
                        .set_name(w.first)
                    ));
        }
        else if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                &w.second.role))
        {
            shm_cfg.features.push_back(
                std::make_shared<snappyHexMeshFeats::RefinementGeometry>(
                    snappyHexMeshFeats::RefinementGeometry::Parameters()
                        .set_scalefactor(p().geometryscale)
                        .set_geometry(w.second.file)
                        .set_level(maxLevel)
                        .set_name(w.first)
                    ));
        }
        else
        {
            auto feat=surfaceFeatureExtract(w.second.file->geometry(), 30.);

            shm_cfg.features.push_back(
                snappyHexMeshFeats::FeaturePtr(
                    new snappyHexMeshFeats::ExplicitFeatureCurve(
                        snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
                            .set_level(maxLevel)
                            .set_scalefactor(p().geometryscale)
                            .set_geometry(make_geometryFile(feat))
                            .set_name(w.first+"_features")
                       )));

            int nLayers=0;
            if (boost::get<Parameters::geometry_default_type::role_wall_type>(&w.second.role))
            {
                nLayers=p().mesh.nLayers;
            }

            shm_cfg.features.push_back(
                snappyHexMeshFeats::FeaturePtr(
                    new snappyHexMeshFeats::Geometry(
                        snappyHexMeshFeats::Geometry::Parameters()
                            .set_minLevel(minLevel)
                            .set_maxLevel(maxLevel)
                            .set_nLayers(nLayers)
                            .set_scalefactor(p().geometryscale)
                           .set_geometry(w.second.file)
                           .set_name(w.first)
                       )));
        }
    }

    shm_cfg.PiM.push_back(p().mesh.PiM);
    shm_cfg.tlayer=p().mesh.tlayer;
    shm_cfg.erlayer=p().mesh.erlayer;
    shm_cfg.relativeSizes=p().mesh.relativeSizes;
    
    snappyHexMesh
    (
          cm, executionPath(),
          shm_cfg,
          true, false, false,
          &pp
    );


    resetMeshToLatestTimestep(cm, executionPath(), true);
      
    cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite"));

}


void InternalPressureLoss::applyCustomPreprocessing(OpenFOAMCase& cm, ProgressDisplayer& progress)
{
    boost::filesystem::path STLOutPath =
        snappyHexMeshFeats::geometryDir(cm, this->executionPath());

    std::vector<std::string> cellSetCmds;

    for (const auto&w: p().geometry)
    {
        if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                &w.second.role))
        {

            auto stl=STLOutPath/(w.first+".stlb");
            cellSetCmds.push_back(
                "cellSet "+w.first+
                " new surfaceToCell \""
                +stl.string()
                +"\" ("
                +OFDictData::toString(
                    OFDictData::vector3(
                        p().mesh.PiM))
                +") 0 1 0 0 0"
                );
        }
    }

    if (cellSetCmds.size())
    {
        setSet(cm, executionPath(), cellSetCmds);
        cm.executeCommand(executionPath(), "setsToZones", { "-noFlipMap" } );
    }
}



void InternalPressureLoss::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer&)
{


    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);

    std::unique_ptr<FVNumerics> numeric;

    if (const auto* iso =
        boost::get<Parameters::operation_type::thermalTreatment_isothermal_type>(
            &p().operation.thermalTreatment))
    {
        if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                &p().operation.timeTreatment))
        {
            numeric.reset(new steadyIncompressibleNumerics(cm));
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
            numeric.reset(new unsteadyIncompressibleNumerics(cm, uinp));
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
                numeric.reset(new buoyantSimpleFoamNumerics(cm, bsfnp));
            }
            else
            {
                numeric.reset(new steadyIncompressibleNumerics(cm));
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
                numeric.reset(new buoyantPimpleFoamNumerics(cm, bpfnp));
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
                numeric.reset(new unsteadyIncompressibleNumerics(cm, uinp));
            }
        }

    }
    else throw insight::UnhandledSelection("thermal solution option");

    auto num = cm.insert(numeric.release());

    if (num->isCompressible())
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


    /***********************************************************************************************
     * boundary conditions
     ***********************************************************************************************/

    for (auto& g: p().geometry)
    {

        /****
         * inlet
         ****/
        if (auto *in =boost::get<Parameters::geometry_default_type::role_inlet_type>(
                &g.second.role))
        {
            // grid needs to be present
            patchArea inletprops(
                OpenFOAMCase(OFEs::get(p().run.OFEname)),
                executionPath(), g.first);
            double D=sqrt(inletprops.A_*4./M_PI);
            double turbI=0.1;
            double turbL=D*0.2;

            auto turb = std::make_shared<turbulenceBC::uniformIntensityAndLengthScale>(
                turbulenceBC::uniformIntensityAndLengthScale::Parameters()
                    .set_I(turbI) .set_l(turbL)
                );

            double T=300.;
            if (const auto* thermsolve =
                boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
                    &p().operation.thermalTreatment))
            {
                auto &tbcc=thermsolve->BCs.at(g.first);
                if (auto* tbc=boost::get<Parameters::operation_type::thermalTreatment_solve_type
                                           ::BCs_default_adiabatic_type>(
                        &tbcc))
                {
                    throw insight::Exception("invalid temperature BC at boundary %s: fixed temperature required",
                                             g.first.c_str());
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

            if (auto* mfl=boost::get<Parameters::geometry_default_type::role_inlet_type
                                       ::specification_massFlow_type>(
                    &in->specification))
            {
                MassflowBC::Parameters inp;
                MassflowBC::Parameters::flowrate_massflow_type mf = { mfl->dotm };
#warning check handling of rho for incompressible case
                inp.flowrate = mf;
                inp.turbulence=turb;
                inp.temperature=MassflowBC::Parameters::temperature_staticTemperature_type{ T };
                cm.insert(new MassflowBC(cm, g.first, boundaryDict, inp));
            }
            else if (auto* vfl=boost::get<Parameters::geometry_default_type::role_inlet_type
                                            ::specification_volumetricFlow_type>(
                         &in->specification))
            {
                MassflowBC::Parameters inp;
                MassflowBC::Parameters::flowrate_volumetric_type mf = { vfl->Q };
                inp.flowrate = mf;
                inp.turbulence=turb;
                inp.temperature=MassflowBC::Parameters::temperature_staticTemperature_type{T};
                cm.insert(new MassflowBC(cm, g.first, boundaryDict, inp));
            }
            else if (auto* vfl=boost::get<Parameters::geometry_default_type::role_inlet_type::specification_vector_type>(
                    &in->specification))
            {
                VelocityInletBC::Parameters inp;
                inp.velocity=FieldData::uniformSteady(vfl->velocity);
                inp.turbulence=turb;
                inp.T=FieldData::uniformSteady(T);
                cm.insert(new VelocityInletBC(cm, g.first, boundaryDict, inp));
            }
            else if (auto* suc=boost::get<Parameters::geometry_default_type::role_inlet_type
                                              ::specification_pressureInlet_type>(
                         &in->specification))
            {
                SuctionInletBC::Parameters inp;
                inp.turb_I=turbI;
                inp.turb_L=turbL;
                inp.pressure=suc->ambientPressure;
                inp.T=T;
                cm.insert(new SuctionInletBC(cm, g.first, boundaryDict, inp));
            }
        }
        /*****
         * outlet
         *****/
        else if (auto *out =boost::get<Parameters::geometry_default_type::role_outlet_type>(
                &g.second.role))
        {
            cm.insert(new PressureOutletBC(
                cm, g.first, boundaryDict, PressureOutletBC::Parameters()
                    .set_behaviour( PressureOutletBC::Parameters::behaviour_uniform_type(
                        FieldData::Parameters()
                            .set_fielddata(FieldData::Parameters::fielddata_uniformSteady_type(
                                vec1(sp().pAmbient_+out->pressure))), false
                        ))
                ));

        }
        /*****
         * wall
         *****/
        else if (auto *wall =boost::get<Parameters::geometry_default_type::role_wall_type>(
                &g.second.role))
        {
            WallBC::Parameters wp;

            if (const auto* thermsolve =
                boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
                    &p().operation.thermalTreatment))
            {
                auto &tbcc=thermsolve->BCs.at(g.first);
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
            cm.insert(new WallBC(cm, g.first, boundaryDict, wp));
        }
        /*****
         * symmetry
         *****/
        else if (auto *symm =boost::get<Parameters::geometry_default_type::role_symmetry_type>(
                     &g.second.role))
        {
            cm.insert(new SymmetryBC(cm, g.first, boundaryDict));
        }
        /*****
         * porous zone
         *****/
        else if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                &g.second.role))
        {
            porousZoneOption::Parameters pzp;
            pzp.set_name(g.first);
            pzp.porousZone.d=vec3(1,1,1)*vol->d/p().fluid.nu;
            pzp.porousZone.f=vec3(1,1,1)*2.*vol->f/p().fluid.rho;

            cm.insert(new porousZoneOption(cm, pzp));
        }
    }


    {
        calculateTotalPressure::Parameters ctp;
        ctp
            .set_pName("p")
            .set_UName("U")
            .set_rho(calculateTotalPressure::Parameters::rho_rhoInf_type
                     {p().fluid.rho}
                     )
            .set_name("calcTotalPressure")
            ;

        if (num->isCompressible())
            ctp.set_rho(calculateTotalPressure::Parameters::rho_field_type
                     {"rho"}
                     );
        else
            ctp.set_rho(calculateTotalPressure::Parameters::rho_rhoInf_type
                     {p().fluid.rho}
                      );

        cm.insert(new calculateTotalPressure( cm, ctp ));
    }

    for (auto& g: p().geometry)
    {

        /****
         * inlet or outlet
         ****/
        if (
            (boost::get<Parameters::geometry_default_type::role_inlet_type>(
                &g.second.role))
            ||
            (boost::get<Parameters::geometry_default_type::role_outlet_type>(
                &g.second.role))
            )
        {
            cm.insert(new surfaceIntegrate(
                cm, surfaceIntegrate::Parameters()
                    .set_domain( surfaceIntegrate::Parameters::domain_patch_type( g.first ) )
                    .set_fields( { "pTotal" } )
                    .set_operation( surfaceIntegrate::Parameters::areaAverage )
                    .set_outputControl("timeStep")
                    .set_outputInterval(1)
                    .set_name(g.first+"_pressure")
                ));
        }
    }


    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p().operation.thermalTreatment))
    {
        for (auto& wbc: thermsolve->BCs)
        {
        }

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

    if (num->isCompressible())
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







RangeParameterList rpl_InternalPressureLossCharacteristics = { "operation/Q" };


defineType(InternalPressureLossCharacteristics);
Analysis::Add<InternalPressureLossCharacteristics> addInternalPressureLossCharacteristics;


InternalPressureLossCharacteristics::InternalPressureLossCharacteristics(
    const std::shared_ptr<supplementedInputDataBase>& sp  )
: OpenFOAMParameterStudy<InternalPressureLoss,rpl_InternalPressureLossCharacteristics>(
          sp )
{}



void InternalPressureLossCharacteristics::evaluateCombinedResults(ResultSet &results)
{
    hierarchicalData::Ordering o(0.1);
    std::vector<std::string> headers = { "delta_p" };

    std::string key="deltaPTable";
    const TabularResult& tab
        = static_cast<const TabularResult&>(
            results.insert
            (
                       key,
                       this->table(
                           "", "", "operation/Q",
                           headers, nullptr,
                           TableInputType::DoubleInputParameter)
                       ).setOrder(o.next()));

    arma::mat tabdat=tab.toMat();

    addPlot
        (
            results, this->executionPath(), "chartDeltaP",
            "$Q / (m^3 s^{-1})$", "$\\Delta_p / Pa$",
            {
                PlotCurve(tabdat, "deltaP", "w l not")
            },
            "Chart of pressure loss vs. volume flux"
            ) .setOrder(o.next());

}




}
