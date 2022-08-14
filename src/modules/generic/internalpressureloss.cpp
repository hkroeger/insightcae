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

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight 
{

addToAnalysisFactoryTable(InternalPressureLoss);


void InternalPressureLoss::modifyDefaults(ParameterSet& p)
{
  p.getBool("run/potentialinit")=true;
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
  if ( const Parameters::geometry_STEP_type* geom_cad =
       boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
    {
      using namespace insight::cad;

      if (!geom_cad->cadmodel->isValid())
        throw insight::Exception("Geometry file does not exist: "+geom_cad->cadmodel->fileName().string());

      FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel->filePath());
      bb_=cadmodel->modelBndBox();

      if ( const Parameters::geometry_STEP_type::inout_extra_files_type* io_extra =
           boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
        {
          if (!io_extra->inlet_model->isValid())
            throw insight::Exception("Geometry file does not exist: "+io_extra->inlet_model->fileName().string());
          {
            FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model->filePath());
            bb_.extend(inletmodel->modelBndBox());
          }

          if (!io_extra->outlet_model->isValid())
            throw insight::Exception("Geometry file does not exist: "+io_extra->outlet_model->fileName().string());
          {
            FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model->filePath());
            bb_.extend(outletmodel->modelBndBox());
          }
        }
    }
  else if ( const Parameters::geometry_STL_type* geom_stl =
            boost::get<Parameters::geometry_STL_type>(&p.geometry) )
    {
      if (!geom_stl->cadmodel->isValid())
        throw insight::Exception("Geometry file does not exist: "+geom_stl->cadmodel->fileName().string());

      bb_ = STLBndBox(readSTL(geom_stl->cadmodel->filePath()));

      {
        if (!geom_stl->inlet->isValid())
          throw insight::Exception("Geometry file does not exist: "+geom_stl->inlet->fileName().string());

        bb_.extend(STLBndBox(readSTL(geom_stl->inlet->filePath())));
      }
      {
        if (!geom_stl->outlet->isValid())
          throw insight::Exception("Geometry file does not exist: "+geom_stl->outlet->fileName().string());

        bb_.extend(STLBndBox(readSTL(geom_stl->outlet->filePath())));
      }
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

    cm.insert(new MeshingNumerics(cm));
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

    if ( const Parameters::geometry_STEP_type* geom_cad =
         boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
      {
        using namespace insight::cad;

        FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel->filePath());

        if ( const Parameters::geometry_STEP_type::inout_named_surfaces_type* io_name =
             boost::get<Parameters::geometry_STEP_type::inout_named_surfaces_type>(&geom_cad->inout) )
          {
              auto inletss=cadmodel->providedSubshapes().find("face_"+io_name->inlet_name);
              if (inletss==cadmodel->providedSubshapes().end())
                  throw insight::Exception("named face \""+io_name->inlet_name+"\" not found in CAD model!");

              FeaturePtr inlet=inletss->second;
              inlet->checkForBuildDuringAccess();

              auto outletss=cadmodel->providedSubshapes().find("face_"+io_name->outlet_name);
              if (outletss==cadmodel->providedSubshapes().end())
                  throw insight::Exception("named face \""+io_name->outlet_name+"\" not found in CAD model!");

              FeaturePtr outlet=outletss->second;
              outlet->checkForBuildDuringAccess();

              FeatureSetParserArgList args;
              args.push_back(cadmodel->providedFeatureSet("face_"+io_name->inlet_name));
              args.push_back(cadmodel->providedFeatureSet("face_"+io_name->outlet_name));
              FeatureSetPtr fp(new FeatureSet(cadmodel, insight::cad::Face, "!( in(%0) || in(%1) )",  args));
              FeaturePtr walls(new Feature(fp));

              walls->saveAs(sp().wallstlfile_);
              inlet->saveAs(sp().inletstlfile_);
              outlet->saveAs(sp().outletstlfile_);
          }
        else if ( const Parameters::geometry_STEP_type::inout_extra_files_type* io_extra =
             boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
          {

            cadmodel->saveAs(sp().wallstlfile_);
            FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model->filePath());
            inletmodel->saveAs(sp().inletstlfile_);
            FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model->filePath());
            outletmodel->saveAs(sp().outletstlfile_);
          }
      }
    else if ( const Parameters::geometry_STL_type* geom_stl = boost::get<Parameters::geometry_STL_type>(&p.geometry) )
      {
        cm.executeCommand(executionPath(), "surfaceConvert", {geom_stl->cadmodel->filePath().string(), sp().wallstlfile_.string()});
        cm.executeCommand(executionPath(), "surfaceConvert", {geom_stl->inlet->filePath().string(), sp().inletstlfile_.string()});
        cm.executeCommand(executionPath(), "surfaceConvert", {geom_stl->outlet->filePath().string(), sp().outletstlfile_.string()});
      }
    
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
        MassflowBC::Parameters::flowrate_massflow_type mf = { p.fluid.rho * p.operation.Q };
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

    ap.message("Producing convergence history plot...");
    addPlot
    (
      results, executionPath(), "chartPressureDifference",
      "Iteration", "$p/\\rho$",
      {
         PlotCurve(p_vs_t.col(0), p_vs_t.col(1), "pmean_vs_iter", "w l not")
      },
      "Plot of pressure difference between inlet and outlet vs. iterations"
    );
  ++ap;

    double delta_p=p_vs_t(p_vs_t.n_rows-1,1)*p.fluid.rho;
    ptr_map_insert<ScalarResult>(*results) ("delta_p", delta_p, "Pressure difference", "", "Pa");

    ap.message("Rendering images...");
    {
      // A renderer and render window
      OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

      auto patches = scene.patches("wall.*");
      auto im = scene.internalMesh();

      FieldSelection sl_field("p", FieldSupport::Point, -1);
      auto sl_range=calcRange(sl_field, {patches}, {});
      auto sl_cm=createColorMap();
      FieldColor sl_fc(sl_field, sl_cm, sl_range);

      double Lmax=p.geometryscale*arma::as_scalar(arma::max(sp().L_));
      arma::mat ctr=p.geometryscale*( sp().bb_.col(1) + sp().bb_.col(0) )*0.5;

      {
        auto seeds = vtkSmartPointer<vtkPointSource>::New();
        seeds->SetCenter(toArray(p.mesh.PiM *p.geometryscale));
        seeds->SetRadius(0.5*arma::norm(sp().L_,2));
        seeds->SetDistributionToUniform();
        seeds->SetNumberOfPoints(500);

        auto st = vtkSmartPointer<vtkStreamTracer>::New();
        st->SetInputData(im);
        st->SetSourceConnection(seeds->GetOutputPort());
        st->SetMaximumPropagation(10.*Lmax);
        st->SetIntegrationDirectionToBoth();
        st->SetInputArrayToProcess(
              0, 0, 0,
              vtkDataObject::FIELD_ASSOCIATION_POINTS,
              "U");

        st->Update();
        scene.addData<vtkPolyDataMapper>(st->GetOutput(), vec3(0.5,0.5,0.5));
      }

      scene.addData<vtkPolyDataMapper>(patches, vec3(0.9, 0.9, 0.9));

      auto camera = scene.activeCamera();
      camera->ParallelProjectionOn();

      camera->SetFocalPoint( toArray(ctr) );

      {
        camera->SetViewUp( toArray(vec3(0,0,1)) );
        camera->SetPosition( toArray(ctr+10.*vec3(-sp().L_[0],-sp().L_[1],sp().L_[2])) );

        auto img = executionPath() / "streamLines_diag.png";
  //      scene.fitAll();
        double f=sqrt(2.);
        scene.setParallelScale(std::pair<double,double>(
                                 std::max(f*sp().L_[0], f*sp().L_[1]),
                                 std::max(f*sp().L_[0], f*sp().L_[2])
                                 ));
        scene.exportImage(img);
        results->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Stream lines (isometric view)", ""
        )));
      }
    ++ap;

      scene.clearScene();

      scene.addData<vtkDataSetMapper>(patches, sl_fc);
      scene.addColorBar("Pressure\n[m^2/s^2]", sl_cm);

      {
        camera->SetViewUp( toArray(vec3(0,0,1)) );
        camera->SetPosition( toArray(ctr+10.*vec3(-sp().L_[0],-sp().L_[1],sp().L_[2])) );

        auto img = executionPath() / "pressure_diag.png";
  //      scene.fitAll();
        double f=sqrt(2.);
        scene.setParallelScale(std::pair<double,double>(
                                 std::max(f*sp().L_[0], f*sp().L_[1]),
                                 std::max(f*sp().L_[0], f*sp().L_[2])
                                 ));
        scene.exportImage(img);
        results->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Pressure (isometric view)", ""
        )));
      }
    ++ap;

      {
        camera->SetViewUp( toArray(vec3(0,0,1)) );
        camera->SetPosition( toArray(ctr+10.*vec3(sp().L_[0],0,0)) );

        auto img = executionPath() / "streamLines_front.png";
        scene.setParallelScale(std::pair<double,double>( sp().L_[1], sp().L_[2]));
        scene.exportImage(img);
        results->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Stream lines (front view)", ""
        )));
      }
    ++ap;

      {
        camera->SetViewUp( toArray(vec3(0,0,1)) );
        camera->SetPosition( toArray(ctr+10.*vec3(0,sp().L_[1],0)) );

        auto img = executionPath() / "streamLines_side.png";
        scene.setParallelScale(std::pair<double,double>( sp().L_[0], sp().L_[2]));
        scene.exportImage(img);
        results->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Stream lines (side view)", ""
        )));
      }
    ++ap;

      {
        camera->SetViewUp( toArray(vec3(0,-1,0)) );
        camera->SetPosition( toArray(ctr+10.*vec3(0,0,sp().L_[2])) );

        auto img = executionPath() / "streamLines_top.png";
        scene.setParallelScale(std::pair<double,double>( sp().L_[0], sp().L_[1]));
        scene.exportImage(img);
        results->insert(img.filename().stem().string(),
          std::unique_ptr<Image>(new Image
          (
          executionPath(), img.filename(),
          "Stream lines (top view)", ""
        )));
      }
    ++ap;

    }
    
    return results;
}


ParameterSetVisualizerPtr InternalPressureLoss_visualizer()
{
    return ParameterSetVisualizerPtr( new InternalPressureLoss_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(Analysis, InternalPressureLoss, visualizer, InternalPressureLoss_visualizer);


void InternalPressureLoss_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CADParameterSetVisualizer::recreateVisualizationElements();

  Parameters p(currentParameters());

  try
  {

    if ( const Parameters::geometry_STEP_type* geom_cad = boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
      {
        using namespace insight::cad;

        if (geom_cad->cadmodel->isValid())
        {
          FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel->filePath());

          if ( const Parameters::geometry_STEP_type::inout_named_surfaces_type* io_name =
               boost::get<Parameters::geometry_STEP_type::inout_named_surfaces_type>(&geom_cad->inout) )
            {
                auto inletss=cadmodel->providedSubshapes().find("face_"+io_name->inlet_name);
                if (inletss==cadmodel->providedSubshapes().end())
                    throw insight::Exception("named face \""+io_name->inlet_name+"\" not found in CAD model!");

                FeaturePtr inlet=inletss->second;
                inlet->checkForBuildDuringAccess();

                auto outletss=cadmodel->providedSubshapes().find("face_"+io_name->outlet_name);
                if (outletss==cadmodel->providedSubshapes().end())
                    throw insight::Exception("named face \""+io_name->outlet_name+"\" not found in CAD model!");

                FeaturePtr outlet=outletss->second;
                outlet->checkForBuildDuringAccess();

                FeatureSetParserArgList args;
                args.push_back(cadmodel->providedFeatureSet("face_"+io_name->inlet_name));
                args.push_back(cadmodel->providedFeatureSet("face_"+io_name->outlet_name));
                FeatureSetPtr fp(new FeatureSet(cadmodel, insight::cad::Face, "!( in(%0) || in(%1) )",  args));
                FeaturePtr walls(new Feature(fp));

                addFeature("walls", walls);
                addFeature("inlet", inlet);
                addFeature("outlet", outlet);
            }
          else if ( const Parameters::geometry_STEP_type::inout_extra_files_type* io_extra =
               boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
            {
              addFeature("walls", cadmodel);
              if (io_extra->inlet_model->isValid())
              {
                FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model->filePath());
                addFeature("inlet", inletmodel);
              }
              if (io_extra->outlet_model->isValid())
              {
                FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model->filePath());
                addFeature("outlet", outletmodel);
              }
            }
        }
      }
    else if ( const Parameters::geometry_STL_type* geom_stl =
              boost::get<Parameters::geometry_STL_type>(&p.geometry) )
      {
        if (geom_stl->cadmodel->isValid())
          addFeature("walls", insight::cad::STL::create(geom_stl->cadmodel->filePath()) );

        if (geom_stl->inlet->isValid())
          addFeature("inlet", insight::cad::STL::create(geom_stl->inlet->filePath()) );

        if (geom_stl->outlet->isValid())
          addFeature("outlet", insight::cad::STL::create(geom_stl->outlet->filePath()) );
      }

    addDatum( "PiM",
              cad::DatumPtr(
                new cad::ExplicitDatumPoint(cad::matconst(p.mesh.PiM))
                ) );
  }
  catch (...)
  {
    // ignore
  }
}

}
