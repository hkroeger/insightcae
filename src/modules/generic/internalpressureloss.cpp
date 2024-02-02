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
#include "openfoam/ofes.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh.h"
#include "openfoam/snappyhexmesh.h"
#include "base/vtkrendering.h"
#include "base/vtktransformation.h"

#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
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

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight 
{

addToAnalysisFactoryTable(InternalPressureLoss);


void InternalPressureLoss::modifyDefaults(ParameterSet& p)
{
    p.setBool("run/potentialinit", true);
}


InternalPressureLoss::supplementedInputData::supplementedInputData(
    std::unique_ptr<Parameters> pPtr,
    const boost::filesystem::path &executionPath,
    ProgressDisplayer &prg )
  : supplementedInputDataDerived<Parameters>( std::move(pPtr) )
{
  auto& p=this->p();

  inletstlfile_=executionPath/"constant"/"triSurface"/"inlet.stlb";
  outletstlfile_=executionPath/"constant"/"triSurface"/"outlet.stlb";
  wallstlfile_=executionPath/"constant"/"triSurface"/"walls.stlb";

  // Analyze geometry
  // Find:
  // * Domain BB
  // * Inlet hydraulic diam.


  prg.message("Analyzing geometry...");
  if ( const auto* geom_cad =
       boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
    {
      using namespace insight::cad;

      if (!geom_cad->cadmodel->isValid())
        throw insight::Exception("Geometry file does not exist: "+geom_cad->cadmodel->fileName().string());

      walls_ = Feature::create(geom_cad->cadmodel->filePath());

      if ( const auto* io_extra =
           boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
        {
          if (!io_extra->inlet_model->isValid())
            throw insight::Exception("Geometry file does not exist: "+io_extra->inlet_model->fileName().string());
          {
            inlet_ = Feature::create(io_extra->inlet_model->filePath());
          }

          if (!io_extra->outlet_model->isValid())
            throw insight::Exception("Geometry file does not exist: "+io_extra->outlet_model->fileName().string());
          {
            outlet_ = Feature::create(io_extra->outlet_model->filePath());
          }
        }
      else if ( const auto* io_name =
          boost::get<Parameters::geometry_STEP_type::inout_named_surfaces_type>(&geom_cad->inout) )
      {
          auto inletss=walls_->providedSubshapes().find("face_"+io_name->inlet_name);
          if (inletss==walls_->providedSubshapes().end())
            throw insight::Exception("named face \""+io_name->inlet_name+"\" not found in CAD model!");

          inlet_=inletss->second;
          inlet_->checkForBuildDuringAccess();

          auto outletss=walls_->providedSubshapes().find("face_"+io_name->outlet_name);
          if (outletss==walls_->providedSubshapes().end())
            throw insight::Exception("named face \""+io_name->outlet_name+"\" not found in CAD model!");

          outlet_=outletss->second;
          outlet_->checkForBuildDuringAccess();

          FeatureSetParserArgList args;
          args.push_back(walls_->providedFeatureSet("face_"+io_name->inlet_name));
          args.push_back(walls_->providedFeatureSet("face_"+io_name->outlet_name));
          FeatureSetPtr fp(new FeatureSet(walls_, insight::cad::Face, "!( in(%0) || in(%1) )",  args));
          walls_ = Feature::create(fp);
      }
    }
  else if ( const auto* geom_stl =
            boost::get<Parameters::geometry_STL_type>(&p.geometry) )
    {
      if (!geom_stl->cadmodel->isValid())
        throw insight::Exception("Geometry file does not exist: "+geom_stl->cadmodel->fileName().string());

      walls_ = cad::STL::create(geom_stl->cadmodel->filePath());

      {
        if (!geom_stl->inlet->isValid())
          throw insight::Exception("Geometry file does not exist: "+geom_stl->inlet->fileName().string());

        inlet_ = cad::STL::create(geom_stl->inlet->filePath());
      }
      {
        if (!geom_stl->outlet->isValid())
          throw insight::Exception("Geometry file does not exist: "+geom_stl->outlet->fileName().string());

        outlet_ = cad::STL::create(geom_stl->outlet->filePath());
      }
    }

  bb_=walls_->modelBndBox();
  if (inlet_) bb_.extend(inlet_->modelBndBox());
  if (outlet_) bb_.extend(outlet_->modelBndBox());

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
                                        .set_np(p.run.np)
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
	    new Block(P_8(
			  pt[0], pt[1], pt[2], pt[3],
			  pt[4], pt[5], pt[6], pt[7]
		      ),
                      sp().nx_, sp().ny_, sp().nz_
		      )
	);
    }
    int nb=bmd->nBlocks();
    cm.insert(bmd.release());
    cm.createOnDisk(executionPath());
    cm.runBlockMesh(executionPath(), nb, &pp);


    create_directory(sp().wallstlfile_.parent_path());

    sp().walls_->saveAs(sp().wallstlfile_);
    sp().inlet_->saveAs(sp().inletstlfile_);
    sp().outlet_->saveAs(sp().outletstlfile_);

    surfaceFeatureExtract(cm, executionPath(), sp().wallstlfile_.filename().string());
    
    snappyHexMeshConfiguration::Parameters shm_cfg;

    arma::mat s = vec3(1,1,1)*p.geometryscale;
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::ExplicitFeatureCurve(snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
      .set_level(p.mesh.maxLevel)
      .set_scale(s)
      .set_fileName(make_filepath(executionPath()/"constant"/"triSurface"/(sp().wallstlfile_.filename().stem().string()+".eMesh")))
    )));

    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("walls")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(p.mesh.nLayers)
      .set_scale(s)

      .set_fileName(make_filepath(sp().wallstlfile_))
    )));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("inlet")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(0)
      .set_scale(s)

      .set_fileName(make_filepath(sp().inletstlfile_))
    )));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("outlet")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(0)
      .set_scale(s)

      .set_fileName(make_filepath(sp().outletstlfile_))
    )));


    shm_cfg.PiM.push_back(p.mesh.PiM);
    
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
    cm.insert(new steadyIncompressibleNumerics(cm));
    cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters() ));
    
    cm.insert(new PressureOutletBC(cm, "outlet", boundaryDict, PressureOutletBC::Parameters()
              .set_behaviour( PressureOutletBC::Parameters::behaviour_uniform_type(
                 FieldData::Parameters()
                  .set_fielddata(FieldData::Parameters::fielddata_uniformSteady_type(vec1(0.0)))
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

    cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );
    
    insertTurbulenceModel(cm, p.fluid.turbulenceModel);
}


ResultSetPtr InternalPressureLoss::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& pp)
{
    auto&p=this->p();

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
      "Iteration", "$p/\\rho$",
      {
         PlotCurve(p_vs_t.col(0), p_vs_t.col(1), "pmean_vs_iter", "w l not")
      },
      "Plot of pressure difference between inlet and outlet vs. iterations",
      str ( format ( "set yrange [%g:%g]" )
            % ( min ( 0.0, 1.1*psig.min() ) )
            % ( 1.1*psig.max() ) )
    );
  ++ap;

    double delta_p=p_vs_t(p_vs_t.n_rows-1,1)*p.fluid.rho;
    ptr_map_insert<ScalarResult>(*results) ("delta_p", delta_p, "Pressure difference", "", "Pa");



    ap.message("Rendering images...");
    {
      // A renderer and render window
      OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

      auto inlet = scene.patchesFilter("inlet.*");
      auto patches = scene.patchesFilter("wall.*");
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
      scene.addColorBar("Pressure\n[m^2/s^2]", p_fc.lookupTable());

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





ParameterSetVisualizerPtr InternalPressureLoss_visualizer()
{
    return ParameterSetVisualizerPtr( new InternalPressureLoss_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(Analysis, InternalPressureLoss, visualizer, InternalPressureLoss_visualizer);


void InternalPressureLoss_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CADParameterSetVisualizer::recreateVisualizationElements();

  try
  {

    auto spp=std::make_shared<InternalPressureLoss::supplementedInputData>(
        std::make_unique<InternalPressureLoss::Parameters>(currentParameters()),
        "", *progress_ );

    Q_EMIT updateSupplementedInputData(std::dynamic_pointer_cast<supplementedInputDataBase>(spp));


    addFeature("walls", spp->walls_, {insight::Surface, vec3(QColorConstants::Gray)});
    addFeature("inlet", spp->inlet_, {insight::Surface, vec3(QColorConstants::Blue)});
    addFeature("outlet", spp->outlet_, {insight::Surface, vec3(QColorConstants::Green)});

    addDatum(
        "PiM",
        std::make_shared<cad::ExplicitDatumPoint>(
            cad::matconst(spp->p().mesh.PiM*1e3) ), true );
  }
  catch (...)
  {
    // ignore
  }
}



}
