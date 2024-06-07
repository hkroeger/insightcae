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

#include "base/factory.h"
#include "base/units.h"
#include "boost/algorithm/string/case_conv.hpp"
#include "boost/algorithm/string/join.hpp"
#include "boost/format/format_fwd.hpp"
#include "boost/iterator_adaptors.hpp"
#include "cadfeature.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
#include "openfoam/caseelements/boundaryconditions/exptdatainletbc.h"
#include "openfoam/caseelements/basic/limitquantities.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"
#include "openfoam/snappyhexmesh.h"
#include "base/vtkrendering.h"
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
#include "openfoam/caseelements/boundaryconditions/massflowbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/analysiscaseelements.h"

#include "cadfeatures.h"
#include "cadfeatures/stl.h"
#include "datum.h"

#include "vtkPointSource.h"
#include "vtkStreamTracer.h"
#include "vtkDataSetMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkMaskPoints.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkIntegrateAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyDataNormals.h"
#include "vtkArrayCalculator.h"

#include <algorithm>
#include <iterator>
#include <memory>

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight 
{

addToAnalysisFactoryTable(InternalPressureLoss);


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


InternalPressureLoss::supplementedInputData::supplementedInputData(
    std::unique_ptr<Parameters> pPtr,
    const boost::filesystem::path &executionPath,
    ProgressDisplayer &prg )
  : supplementedInputDataDerived<Parameters>( std::move(pPtr) )
{
  auto& p=this->p();


  stldir_=boost::filesystem::path("constant")/"triSurface";
  fn_inlet_="inlet";
  fn_outlet_="outlet";

  // Analyze geometry
  // Find:
  // * Domain BB
  // * Inlet hydraulic diam.

  auto loadgeom = [](const boost::filesystem::path& fp) -> cad::FeaturePtr
  {
      auto ext = boost::to_lower_copy(
          fp.extension().string());
      if ( (ext==".stl")|(ext==".stlb") )
      {
          // STL file
          return cad::STL::create(fp);
      }
      else
      {
          // CAD exchange format
          return cad::Feature::create(fp);
      }
  };

  prg.message("Analyzing geometry...");
  for (auto &w: p.geometry.walls)
  {
      auto file = w.second.file;
      if (file->isValid())
      {
          cad::FeaturePtr f = loadgeom(file->filePath());
          walls_[w.first]=f;
          if (bb_.empty())
              bb_=f->modelBndBox();
          else
              bb_.extend(f->modelBndBox());
      }
  }

  if (p.geometry.inlet->isValid())
  {
      cad::FeaturePtr f = loadgeom(p.geometry.inlet->filePath());
      inlet_=f;
      bb_.extend(f->modelBndBox());
  }

  if (p.geometry.outlet->isValid())
  {
      cad::FeaturePtr f = loadgeom(p.geometry.outlet->filePath());
      outlet_=f;
      bb_.extend(f->modelBndBox());
  }


  L_=bb_.col(1)-bb_.col(0);

  if (L_(0)<1e-12)
    throw insight::Exception("model size in x direction is zero!");
  if (L_(1)<1e-12)
    throw insight::Exception("model size in y direction is zero!");
  if (L_(2)<1e-12)
    throw insight::Exception("model size in z direction is zero!");

  nx_=std::max(1, int(ceil(L_(0)/p.mesh.size)));
  ny_=std::max(1, int(ceil(L_(1)/p.mesh.size)));
  nz_=std::max(1, int(ceil(L_(2)/p.mesh.size)));

  pAmbient_=0.;

  if (const auto* thermsolve =
      boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
          &p.operation.thermalTreatment))
  {
      globalTmin=globalTmax=thermsolve->initialInternalTemperature*si::degK;
      auto updateTmima = [&](si::Temperature T)
      {
          globalTmin=std::min(globalTmin, T);
          globalTmin=std::min(globalTmin, T);
      };

      updateTmima(thermsolve->inletTemperature*si::degK);

      for (auto& wbc: thermsolve->wallBCs)
      {
          WallBC::Parameters wp;
          if (const auto* wallfixedT =
              boost::get<Parameters::operation_type::thermalTreatment_solve_type::wallBCs_default_fixedTemperature_type>(
                  &wbc.second))
          {
              updateTmima(wallfixedT->wallTemperature*si::degK);
          }
      }

      globalTmin *= 0.5;
      globalTmax *= 1.5;


      reportSupplementQuantity("globalTmin", toValue(globalTmin, si::degK), "lower temperature clip value", "K");
      reportSupplementQuantity("globalTmax", toValue(globalTmax, si::degK), "upper temperature clip value", "K");

      if (const auto *buoy =
          boost::get<Parameters::operation_type::thermalTreatment_solve_type::includeBuoyancy_yes_type>(
              &thermsolve->includeBuoyancy))
      {
          pAmbient_=buoy->outletPressure;
      }
  }
}




InternalPressureLoss::InternalPressureLoss(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& pd)
: OpenFOAMAnalysis
  (
    "Internal Pressure Loss",
    "Determination of internal pressure loss by CFD a simulation",
    ps, exepath
  ),
  parameters_(new supplementedInputData(std::make_unique<Parameters>(ps), exepath, pd))
  // default values for derived parameters
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
  auto& p=this->p();

  cm.insert(new MeshingNumerics(cm, MeshingNumerics::Parameters()
                                        .set_np(np())
                                ));
    cm.createOnDisk(executionPath());
    
    using namespace insight::bmd;
    std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
    bmd->setScaleFactor(p.geometryscale);
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
    arma::mat s = vec3(1,1,1)*p.geometryscale;

    for (const auto&w: sp().walls_)
    {
        auto fn=executionPath()/sp().stldir_/(w.first+".stlb");

        w.second->saveAs(fn);

        surfaceFeatureExtract(cm, executionPath(), fn.filename().string());
        auto efn=fn; efn.replace_extension(".eMesh");

        shm_cfg.features.push_back(
            snappyHexMeshFeats::FeaturePtr(
                new snappyHexMeshFeats::ExplicitFeatureCurve(
                    snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
                       .set_level(p.mesh.maxLevel)
                       .set_scale(s)
                        .set_fileName(make_filepath(efn))
                   )));
        shm_cfg.features.push_back(
            snappyHexMeshFeats::FeaturePtr(
                new snappyHexMeshFeats::Geometry(
                    snappyHexMeshFeats::Geometry::Parameters()
                       .set_name(w.first)
                       .set_minLevel(p.mesh.minLevel)
                       .set_maxLevel(p.mesh.maxLevel)
                       .set_nLayers(p.mesh.nLayers)
                       .set_scale(s)
                       .set_fileName(make_filepath(fn))
                   )));
    }

    sp().inlet_->saveAs(executionPath()/sp().stldir_/(sp().fn_inlet_+".stlb"));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("inlet")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(0)
      .set_scale(s)

      .set_fileName(make_filepath(executionPath()/sp().stldir_/(sp().fn_inlet_+".stlb")))
    )));

    sp().outlet_->saveAs(executionPath()/sp().stldir_/(sp().fn_outlet_+".stlb"));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("outlet")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(0)
      .set_scale(s)

      .set_fileName(make_filepath(executionPath()/sp().stldir_/(sp().fn_outlet_+".stlb")))
    )));


    shm_cfg.PiM.push_back(p.mesh.PiM);
    shm_cfg.tlayer=p.mesh.tlayer;
    shm_cfg.erlayer=p.mesh.erlayer;
    shm_cfg.relativeSizes=p.mesh.relativeSizes;
    
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





void InternalPressureLoss::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer&)
{
  auto&p=this->p();
    // grid needs to be present
    patchArea inletprops(OpenFOAMCase(OFEs::get(p.run.OFEname)), executionPath(), "inlet");
//    double Ain=inletprops.A_;
    double D=sqrt(inletprops.A_*4./M_PI);

    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);

    std::unique_ptr<FVNumerics> numeric;

    if (const auto* iso =
        boost::get<Parameters::operation_type::thermalTreatment_isothermal_type>(
            &p.operation.thermalTreatment))
    {
        if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                &p.operation.timeTreatment))
        {
            numeric.reset(new steadyIncompressibleNumerics(cm));
        }
        else if (const auto* unsteady =
                   boost::get<Parameters::operation_type::timeTreatment_unsteady_type>(
                       &p.operation.timeTreatment))
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
            &p.operation.thermalTreatment))
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
                    &p.operation.timeTreatment))
            {
                psp.underrelax=0.7;
            }
            cm.insert(new PassiveScalar(cm, psp));
        }

        if (boost::get<Parameters::operation_type::timeTreatment_steady_type>(
                &p.operation.timeTreatment))
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
                       &p.operation.timeTreatment) )
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


    cm.insert(new PressureOutletBC(cm, "outlet", boundaryDict, PressureOutletBC::Parameters()
              .set_behaviour( PressureOutletBC::Parameters::behaviour_uniform_type(
                 FieldData::Parameters()
                  .set_fielddata(FieldData::Parameters::fielddata_uniformSteady_type(vec1(sp().pAmbient_)))
                ))
              ));

    {
        MassflowBC::Parameters inp;
        MassflowBC::Parameters::flowrate_volumetric_type mf = { p.operation.Q };
        inp.flowrate = mf;
        inp.turbulence.reset(new turbulenceBC::uniformIntensityAndLengthScale(
                              turbulenceBC::uniformIntensityAndLengthScale::Parameters()
                               .set_I(0.1)
                               .set_l(D*0.2)
        ));
        if (const auto* thermsolve =
            boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
                &p.operation.thermalTreatment))
        {
            inp.set_T(thermsolve->inletTemperature);
        }
        cm.insert(new MassflowBC(cm, "inlet", boundaryDict, inp));
    }
        
    cm.insert(new surfaceIntegrate(cm, surfaceIntegrate::Parameters()
                 .set_domain( surfaceIntegrate::Parameters::domain_patch_type( "inlet" ) )
                 .set_fields( { "p" } )
                 .set_operation( surfaceIntegrate::Parameters::areaAverage )
                 .set_name("inlet_pressure")
                 .set_outputControl("timeStep")
                 .set_outputInterval(1)
    ));

    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p.operation.thermalTreatment))
    {
        for (auto& wbc: thermsolve->wallBCs)
        {
            WallBC::Parameters wp;
            if (const auto* wallfixedT =
                boost::get<Parameters::operation_type::thermalTreatment_solve_type::wallBCs_default_fixedTemperature_type>(
                    &wbc.second))
            {
                wp.set_heattransfer(
                    std::make_shared<HeatBC::FixedTemperatureBC>(
                     HeatBC::FixedTemperatureBC::Parameters()
                            .set_T( FieldData::uniformSteady(vec1(wallfixedT->wallTemperature)) )
                    ));
            }
            //else adiabtic (is default of WallBC)
            cm.insert(new WallBC(cm, wbc.first, boundaryDict, wp));
        }

        cm.insert(new surfaceIntegrate(cm, surfaceIntegrate::Parameters()
               .set_domain( surfaceIntegrate::Parameters::domain_patch_type( "outlet" ) )
               .set_fields( { "T" } )
               .set_operation( surfaceIntegrate::Parameters::areaAverage )
               .set_name("outlet_temperature")
               .set_outputControl("timeStep")
               .set_outputInterval(1)
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
    
    insertTurbulenceModel(cm, p.fluid.turbulenceModel);
}




ResultSetPtr InternalPressureLoss::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& pp)
{
    auto&p=this->p();

    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);
    auto& numerics = cm.getUniqueElement<FVNumerics>();

    ResultSetPtr results=insight::OpenFOAMAnalysis::evaluateResults(cm, pp);

    auto ap = pp.forkNewAction(8, "Evaluation");

    ap.message("Computing average surface pressure...");
    arma::mat p_vs_t = surfaceIntegrate::readSurfaceIntegrate(cm, executionPath(), "inlet_pressure");
  ++ap;

    arma::mat psig = p_vs_t.rows(p_vs_t.n_rows/3, p_vs_t.n_rows-1).col(1);

    ap.message("Producing convergence history plot...");
    addPlot
    (
      results, executionPath(), "chartPressureDifference",
      "Iteration", numerics.isCompressible()?"$p$":"$p/\\rho$",
      {
         PlotCurve(p_vs_t.col(0), p_vs_t.col(1), "pmean_vs_iter", "w l not")
      },
      "Plot of pressure difference between inlet and outlet vs. iterations",
      str ( format ( "set yrange [%g:%g]" )
            % ( min ( 0.0, 1.1*psig.min() ) )
            % ( 1.1*psig.max() ) )
    );
  ++ap;

    double delta_p=
      p_vs_t(p_vs_t.n_rows-1,1) * (numerics.isCompressible()? 1.0 : p.fluid.rho)
       -
      sp().pAmbient_;

    ptr_map_insert<ScalarResult>(*results) ("delta_p", delta_p, "Pressure difference", "", "Pa");


    if (const auto* thermsolve =
        boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
            &p.operation.thermalTreatment))
    {
        ap.message("Computing average outlet temperature...");
        arma::mat T_vs_t = surfaceIntegrate::readSurfaceIntegrate(cm, executionPath(), "outlet_temperature");
        ++ap;

        arma::mat Tsig = T_vs_t.rows(T_vs_t.n_rows/3, T_vs_t.n_rows-1).col(1);

        ap.message("Producing temperature convergence history plot...");
        addPlot
            (
                results, executionPath(), "chartTemperature",
                "Iteration", "$T/K$",
                {
                    PlotCurve(T_vs_t.col(0), T_vs_t.col(1), "Tmean_vs_iter", "w l not")
                },
                "Plot of temperature, spatially averaged over outlet, vs. iterations",
                str ( format ( "set yrange [%g:%g]" )
                    % ( min ( 0.0, 1.1*Tsig.min() ) )
                    % ( 1.1*Tsig.max() ) )
                );
        ++ap;

        double Tfinal=T_vs_t(T_vs_t.n_rows-1,1);
        ptr_map_insert<ScalarResult>(*results) ("Tfinal", Tfinal, "Temperature in outlet", "", "K");
    }

    ap.message("Rendering images...");
    {
      // A renderer and render window
      OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

      auto inlet = scene.patchesFilter("inlet");
      auto outlet = scene.patchesFilter("outlet");
      std::vector<std::string> wallPatchNames;
      for (const auto&p: boundaryDict)
      {
          auto name = p.first;
          auto patchDict = boost::get<OFDictData::dict>(p.second);
          if (patchDict.getString("type")=="wall")
          {
              wallPatchNames.push_back(name);
          }
      }
      auto patches = scene.patchesFilter(
          boost::join(wallPatchNames, "|") );
      auto internal = scene.internalMeshFilter();


      // use domain center instead of input geometry center,
      // since much of the input geometry might not have been
      // included in the actual domain
      arma::mat bb = PolyDataBndBox(internal->GetOutput());

      FieldSelection p_field("p", FieldSupport::OnPoint, -1);
      FieldColor p_fc(p_field, createColorMap(), calcRange(p_field, {}, {patches}));

      FieldSelection U_field("U", FieldSupport::OnPoint, -1);
      FieldColor U_fc(p_field, createColorMap(), calcRange(p_field, {}, {internal}));

      arma::mat L=p.geometryscale*sp().L_;
      double Lmax=p.geometryscale*arma::as_scalar(arma::max(sp().L_));

      CoordinateSystem objCS(
          ( bb.col(1) + bb.col(0) )*0.5,
          vec3X(), vec3Z() );

      auto views = generateStandardViews(
          objCS,
          Lmax );

      auto camera = scene.activeCamera();
      camera->ParallelProjectionOn();



      // display streamtracers, colored by velocity
      {
        auto seeds = vtkSmartPointer<vtkMaskPoints>::New();
        seeds->SetInputConnection(inlet->GetOutputPort());
        seeds->SetMaximumNumberOfPoints(100);
        seeds->SetRandomMode(true);
        seeds->SetRandomModeType(1);

        auto st = vtkSmartPointer<vtkStreamTracer>::New();
        st->SetInputConnection(internal->GetOutputPort());
        st->SetSourceConnection(seeds->GetOutputPort());
        st->SetMaximumPropagation(10.*Lmax);
        st->SetIntegrationDirectionToBoth();
        st->SetInputArrayToProcess(
              0, 0, 0,
              vtkDataObject::FIELD_ASSOCIATION_POINTS,
              "U");

        scene.addAlgo<vtkDataSetMapper>(st, U_fc);
      }

      // display walls, transparent, gray color
      auto pa = scene.addAlgo<vtkDataSetMapper>(patches, vec3(0.7, 0.7, 0.7));
      pa->GetProperty()->SetOpacity(0.1);

      auto sec_sl = std::make_shared<ResultSection>("Streamlines");
      for (const auto& lv: views)
      {
        scene.setupActiveCamera(lv.second);
        scene.fitAll();

        auto img = executionPath() / ("streamLines_"+lv.first+".png");
        scene.exportImage(img);
        sec_sl->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Stream lines ("+lv.second.title+")", ""
        )));

      ++ap;
      }
      results->insert("streamlines", sec_sl);




      scene.clearScene();

      scene.addAlgo<vtkDataSetMapper>(patches, p_fc);
      scene.addColorBar(
          numerics.isCompressible() ? "Pressure\n[Pa]" : "Pressure\n[m^2/s^2]",
          p_fc.lookupTable());

      auto sec_pres = std::make_shared<ResultSection>("Pressure on walls");
      for (const auto& lv: views)
      {
        scene.setupActiveCamera(lv.second);
        scene.fitAll();

        auto img = executionPath() / ("pressure_"+lv.first+".png");
        scene.exportImage(img);
        sec_pres->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Pressure on walls ("+lv.second.title+")", ""
        )));

      ++ap;
      }
      results->insert("pressure_walls", sec_pres);



      auto cutplane1 = vtkSmartPointer<vtkCutter>::New();
      auto cutplane3 = vtkSmartPointer<vtkCutter>::New();
      auto cutplane2 = vtkSmartPointer<vtkCutter>::New();

      {
        cutplane1->SetInputConnection(internal->GetOutputPort());

        auto slpl = vtkSmartPointer<vtkPlane>::New();
        slpl->SetOrigin(toArray(objCS.origin));
        slpl->SetNormal(toArray(objCS.ex));
        cutplane1->SetCutFunction(slpl);
      }
      {
        cutplane2->SetInputConnection(internal->GetOutputPort());

        auto slpl = vtkSmartPointer<vtkPlane>::New();
        slpl->SetOrigin(toArray(objCS.origin));
        slpl->SetNormal(toArray(objCS.ey));
        cutplane2->SetCutFunction(slpl);
      }
      {
        cutplane3->SetInputConnection(internal->GetOutputPort());

        auto slpl = vtkSmartPointer<vtkPlane>::New();
        slpl->SetOrigin(toArray(objCS.origin));
        slpl->SetNormal(toArray(objCS.ez));
        cutplane3->SetCutFunction(slpl);
      }

      scene.clearScene();

      scene.addAlgo<vtkDataSetMapper>(cutplane1, U_fc);
      scene.addAlgo<vtkDataSetMapper>(cutplane2, U_fc);
      scene.addAlgo<vtkDataSetMapper>(cutplane3, U_fc);
      scene.addColorBar("Velocity\n[m/s]", U_fc.lookupTable());


      auto sec_u = std::make_shared<ResultSection>("Velocity in cut planes");
      for (const auto& lv: views)
      {
      scene.setupActiveCamera(lv.second);
      scene.fitAll();

      auto img = executionPath() / ("velocity_cut_"+lv.first+".png");
      scene.exportImage(img);
      sec_u->insert(
          img.filename().stem().string(),
          std::make_unique<Image>(
              executionPath(), img.filename(),
              "Velocity in cut planes ("+lv.second.title+")", ""
              ));

      ++ap;
      }
      results->insert("velocity_cutplanes", sec_u);


      scene.clearScene();

      scene.addAlgo<vtkDataSetMapper>(cutplane1, p_fc);
      scene.addAlgo<vtkDataSetMapper>(cutplane2, p_fc);
      scene.addAlgo<vtkDataSetMapper>(cutplane3, p_fc);
      scene.addColorBar("Pressure\n[m^2/s^2]", p_fc.lookupTable());

      auto sec_pc = std::make_shared<ResultSection>("Pressure in cut planes");
      for (const auto& lv: views)
      {
      scene.setupActiveCamera(lv.second);
      scene.fitAll();

      auto img = executionPath() / ("pressure_cut_"+lv.first+".png");
      scene.exportImage(img);
      sec_pc->insert(
          img.filename().stem().string(),
          std::make_unique<Image>(
              executionPath(), img.filename(),
              "Pressure in cut planes ("+lv.second.title+")", ""
              ));

      ++ap;
      }
      results->insert("pressure_cutplanes", sec_pc);


      if (const auto* thermsolve =
          boost::get<Parameters::operation_type::thermalTreatment_solve_type>(
              &p.operation.thermalTreatment))
      {


          FieldSelection T_field("T", FieldSupport::OnPoint, -1);
          FieldColor T_fc(T_field,
                          createColorMap(colorMapData_CoolToWarm, 32, true),
                          calcRange(T_field, {}, {internal})
                          );

          scene.clearScene();

          scene.addAlgo<vtkDataSetMapper>(cutplane1, T_fc);
          scene.addAlgo<vtkDataSetMapper>(cutplane2, T_fc);
          scene.addAlgo<vtkDataSetMapper>(cutplane3, T_fc);
          scene.addColorBar("Temperature\n[K]", T_fc.lookupTable());


          auto sec_T = std::make_shared<ResultSection>("Temperature in cut planes");
          for (const auto& lv: views)
          {
              scene.setupActiveCamera(lv.second);
              scene.fitAll();

              auto img = executionPath() / ("temperature_cut_"+lv.first+".png");
              scene.exportImage(img);
              sec_T->insert(
                  img.filename().stem().string(),
                  std::make_unique<Image>(
                      executionPath(), img.filename(),
                      "Temperature in cut planes ("+lv.second.title+")", ""
                      ));

              ++ap;
          }
          results->insert("temperature_cutplanes", sec_T);

          scene.clearScene();

          // display streamtracers, colored by temperature
          {
              auto seeds = vtkSmartPointer<vtkMaskPoints>::New();
              seeds->SetInputConnection(inlet->GetOutputPort());
              seeds->SetMaximumNumberOfPoints(100);
              seeds->SetRandomMode(true);
              seeds->SetRandomModeType(1);

              auto st = vtkSmartPointer<vtkStreamTracer>::New();
              st->SetInputConnection(internal->GetOutputPort());
              st->SetSourceConnection(seeds->GetOutputPort());
              st->SetMaximumPropagation(10.*Lmax);
              st->SetIntegrationDirectionToBoth();
              st->SetInputArrayToProcess(
                  0, 0, 0,
                  vtkDataObject::FIELD_ASSOCIATION_POINTS,
                  "U");

              // display streamlines
              scene.addAlgo<vtkDataSetMapper>(st, T_fc);
          }

          // display walls, transparent, gray color
          auto pa = scene.addAlgo<vtkDataSetMapper>(patches, vec3(0.7, 0.7, 0.7));
          pa->GetProperty()->SetOpacity(0.1);

          auto sec_slt = std::make_shared<ResultSection>("Streamlines with Temperature");
          for (const auto& lv: views)
          {
              scene.setupActiveCamera(lv.second);
              scene.fitAll();

              auto img = executionPath() / ("streamLinesTemp_"+lv.first+".png");
              scene.exportImage(img);
              sec_slt->insert(img.filename().stem().string(),
                             std::unique_ptr<Image>(new Image
                            (
                                executionPath(), img.filename(),
                                "Stream lines with temperature ("+lv.second.title+")", ""
                                )));

              ++ap;
          }
          results->insert("streamlines_temperature", sec_slt);




          T_fc = FieldColor(T_field,
                          createColorMap(colorMapData_CoolToWarm, 32, true),
                          calcRange(T_field, {}, {outlet})
                          );



          auto sec_To = std::make_shared<ResultSection>("Temperature in outlet");

          forEachUnconnectedPart(
              scene, executionPath(), sec_To.get(), outlet,
            [&](vtkAlgorithm* region, ResultSection* sec, int i )
            {

              scene.addAlgo<vtkDataSetMapper>(region, T_fc);
              scene.addColorBar("Temperature\n[K]", T_fc.lookupTable());

              for (const auto& lv: views)
              {
                  if (lv.first=="front" || lv.first=="left" || lv.first=="above")
                  {
                      scene.setupActiveCamera(lv.second);
                      scene.fitAll();

                      auto img = executionPath() /
                                 ("temperature_outlet_"
                                    +lv.first
                                    +(i>=0?"_"+lexical_cast<std::string>(i+1):"")
                                    +".png");

                      scene.exportImage(img);
                      sec->insert(
                          img.filename().stem().string(),
                          std::make_unique<Image>(
                              executionPath(), img.filename(),
                              "Temperature in outlet ("+lv.second.title+")", ""
                              ));

                      ++ap;
                  }
              }


              // display streamtracers, colored by temperature
              {
                  auto seeds = vtkSmartPointer<vtkMaskPoints>::New();
                  seeds->SetInputConnection(region->GetOutputPort());
                  seeds->SetMaximumNumberOfPoints(100);
                  seeds->SetRandomMode(true);
                  seeds->SetRandomModeType(1);

                  auto st = vtkSmartPointer<vtkStreamTracer>::New();
                  st->SetInputConnection(internal->GetOutputPort());
                  st->SetSourceConnection(seeds->GetOutputPort());
                  st->SetMaximumPropagation(10.*Lmax);
                  st->SetIntegrationDirectionToBoth();
                  st->SetInputArrayToProcess(
                      0, 0, 0,
                      vtkDataObject::FIELD_ASSOCIATION_POINTS,
                      "U");

                  // display walls, transparent, gray color
                  auto pa = scene.addAlgo<vtkDataSetMapper>(patches, vec3(0.7, 0.7, 0.7));
                  pa->GetProperty()->SetOpacity(0.1);

                  scene.addAlgo<vtkDataSetMapper>(st, T_fc);

                  scene.addColorBar("Temperature\n[K]", T_fc.lookupTable());

                  for (const auto& lv: views)
                  {
                      if (lv.first=="diag1")
                      {
                          scene.setupActiveCamera(lv.second);
                          scene.fitAll();

                          auto img = executionPath() /
                                     ("streamlines_outlet_"
                                      +lv.first
                                      +(i>=0?"_"+lexical_cast<std::string>(i+1):"")
                                      +".png");

                          scene.exportImage(img);
                          sec->insert(
                              img.filename().stem().string(),
                              std::make_unique<Image>(
                                  executionPath(), img.filename(),
                                  "Streamlines from outlet ("+lv.second.title+")", ""
                                  ));

                          ++ap;
                      }
                  }
              }

              auto es = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
              es->SetInputConnection(region->GetOutputPort());

              auto normals=vtkSmartPointer<vtkPolyDataNormals>::New();
              normals->SetInputConnection(es->GetOutputPort());
              normals->ComputeCellNormalsOn();
              normals->SplittingOn();
              normals->ConsistencyOn();

              auto sfCalc=vtkSmartPointer<vtkArrayCalculator>::New();
              sfCalc->SetInputConnection(normals->GetOutputPort());
              sfCalc->SetAttributeTypeToCellData();
              sfCalc->AddVectorVariable("Normals", "Normals");
              sfCalc->AddVectorVariable("U", "U");
              sfCalc->SetFunction("U.Normals");
              sfCalc->SetResultArrayName( "Flux" );
              sfCalc->Update();

              auto integ = vtkSmartPointer<vtkIntegrateAttributes>::New();
              integ->SetInputConnection(sfCalc->GetOutputPort());
              integ->Update();

              double Tmean =
                  integ->GetOutput()->GetCellData()->GetArray("T")->GetTuple1(0)
                  /
                  integ->GetOutput()->GetCellData()->GetArray("Area")->GetTuple1(0);
              sec->insert("Tmean",
                          new ScalarResult(Tmean, "mean temperature", "", "K") );

              double flux = integ->GetOutput()->GetCellData()->GetArray("Flux")->GetTuple1(0);
              sec->insert("flux",
                          new ScalarResult(flux, "volume flux", "", "m^3/s") );
          });

          results->insert("temperature_outlet", sec_To);
      }
    }




    
    return results;
}




RangeParameterList rpl_InternalPressureLossCharacteristics = { "operation/Q" };


addToAnalysisFactoryTable(InternalPressureLossCharacteristics);


InternalPressureLossCharacteristics::InternalPressureLossCharacteristics(
    const ParameterSet &ps,
    const path &exepath,
    ProgressDisplayer &pd )
: OpenFOAMParameterStudy<InternalPressureLoss,rpl_InternalPressureLossCharacteristics>(
        typeName, "Internal pressure loss calculation for multiple volume fluxes", ps, exepath, pd )
{}



void InternalPressureLossCharacteristics::evaluateCombinedResults(ResultSetPtr &results)
{
    Ordering o(0.1);
    std::vector<std::string> headers = { "delta_p" };

    std::string key="deltaPTable";
    const TabularResult& tab
        = static_cast<const TabularResult&>(
            results->insert
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
