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
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/blockmesh.h"
#include "openfoam/snappyhexmesh.h"
#include "openfoam/paraview.h"

#include "cadfeature.h"
#include "stl.h"
#include "datum.h"

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


InternalPressureLoss::InternalPressureLoss(const ParameterSet& ps, const boost::filesystem::path& exepath)
: OpenFOAMAnalysis
  (
    "Internal Pressure Loss",
    "Determination of internal pressure loss by CFD a simulation",
    ps, exepath
  )
  // default values for derived parameters
{}



void extendBB(arma::mat& bb, const arma::mat& bb2)
{
  for (arma::uword i=0; i<3; i++)
  {
   bb(i,0)=std::min(bb(i,0), bb2(i,0));
   bb(i,1)=std::max(bb(i,1), bb2(i,1));
  }
}

void InternalPressureLoss::calcDerivedInputData()
{
    insight::OpenFOAMAnalysis::calcDerivedInputData();
    Parameters p(parameters_);
    //reportIntermediateParameter("L", L_, "total domain length", "m");

    inletstlfile_=executionPath()/"constant"/"triSurface"/"inlet.stlb";
    outletstlfile_=executionPath()/"constant"/"triSurface"/"outlet.stlb";
    wallstlfile_=executionPath()/"constant"/"triSurface"/"walls.stlb";

    // Analyze geometry
    // Find:
    // * Domain BB
    // * Inlet hydraulic diam.
    
    if ( const Parameters::geometry_STEP_type* geom_cad =
         boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
      {
        using namespace insight::cad;

        if (!boost::filesystem::exists(geom_cad->cadmodel))
          throw insight::Exception("Geometry file does not exist: "+geom_cad->cadmodel.string());

        FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel);
        bb_=cadmodel->modelBndBox();

        if ( const Parameters::geometry_STEP_type::inout_extra_files_type* io_extra =
             boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
          {
            if (!boost::filesystem::exists(io_extra->inlet_model))
              throw insight::Exception("Geometry file does not exist: "+io_extra->inlet_model.string());
            {
              FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model);
              arma::mat bb = inletmodel->modelBndBox();
              extendBB(bb_, bb);
            }

            if (!boost::filesystem::exists(io_extra->outlet_model))
              throw insight::Exception("Geometry file does not exist: "+io_extra->outlet_model.string());
            {
              FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model);
              arma::mat bb = outletmodel->modelBndBox();
              extendBB(bb_, bb);
            }
          }
      }
    else if ( const Parameters::geometry_STL_type* geom_stl =
              boost::get<Parameters::geometry_STL_type>(&p.geometry) )
      {
        if (!boost::filesystem::exists(geom_stl->cadmodel))
          throw insight::Exception("Geometry file does not exist: "+geom_stl->cadmodel.string());

        bb_ = STLBndBox(readSTL(geom_stl->cadmodel));

        {
          if (!boost::filesystem::exists(geom_stl->inlet))
            throw insight::Exception("Geometry file does not exist: "+geom_stl->inlet.string());

          arma::mat bb = STLBndBox(readSTL(geom_stl->inlet));
          extendBB(bb_, bb);
        }
        {
          if (!boost::filesystem::exists(geom_stl->outlet))
            throw insight::Exception("Geometry file does not exist: "+geom_stl->outlet.string());

          arma::mat bb = STLBndBox(readSTL(geom_stl->outlet));
          extendBB(bb_, bb);
        }
      }

    L_=bb_.col(1)-bb_.col(0);
    reportIntermediateParameter("Lx", L_(0), "model size in x direction", "m");
    reportIntermediateParameter("Ly", L_(1), "model size in y direction", "m");
    reportIntermediateParameter("Lz", L_(2), "model size in z direction", "m");

    if (L_(0)<1e-12)
      throw insight::Exception("model size in x direction is zero!");
    if (L_(1)<1e-12)
      throw insight::Exception("model size in y direction is zero!");
    if (L_(2)<1e-12)
      throw insight::Exception("model size in z direction is zero!");

    nx_=std::max(1, int(ceil(L_(0)/p.mesh.size)));
    ny_=std::max(1, int(ceil(L_(1)/p.mesh.size)));
    nz_=std::max(1, int(ceil(L_(2)/p.mesh.size)));
    reportIntermediateParameter("nx", nx_, "initial grid cell numbers in direction x");
    reportIntermediateParameter("ny", ny_, "initial grid cell numbers in direction y");
    reportIntermediateParameter("nz", nz_, "initial grid cell numbers in direction z");

}




void InternalPressureLoss::createMesh(insight::OpenFOAMCase& cm)
{
    Parameters p(parameters_);

    cm.insert(new MeshingNumerics(cm));
    cm.createOnDisk(executionPath());
    
    using namespace insight::bmd;
    std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
    bmd->setScaleFactor(p.geometryscale);
    bmd->setDefaultPatch("walls", "wall");

    double eps=0.01*arma::min(bb_.col(1)-bb_.col(0));
    std::map<int, Point> pt = boost::assign::map_list_of
          (0, 	vec3(bb_(0,0)-eps, bb_(1,0)-eps, bb_(2,0)-eps))
          (1, 	vec3(bb_(0,1)+eps, bb_(1,0)-eps, bb_(2,0)-eps))
          (2, 	vec3(bb_(0,1)+eps, bb_(1,1)+eps, bb_(2,0)-eps))
          (3, 	vec3(bb_(0,0)-eps, bb_(1,1)+eps, bb_(2,0)-eps))
          (4, 	vec3(bb_(0,0)-eps, bb_(1,0)-eps, bb_(2,1)+eps))
          (5, 	vec3(bb_(0,1)+eps, bb_(1,0)-eps, bb_(2,1)+eps))
          (6, 	vec3(bb_(0,1)+eps, bb_(1,1)+eps, bb_(2,1)+eps))
          (7, 	vec3(bb_(0,0)-eps, bb_(1,1)+eps, bb_(2,1)+eps))
          .convert_to_container<std::map<int, Point> >()
          ;

    // create patches
    {
        bmd->addBlock
	(
	    new Block(P_8(
			  pt[0], pt[1], pt[2], pt[3],
			  pt[4], pt[5], pt[6], pt[7]
		      ),
		      nx_, ny_, nz_
		      )
	);
    }
    cm.insert(bmd.release());
    cm.createOnDisk(executionPath());
    cm.executeCommand(executionPath(), "blockMesh");


    create_directory(wallstlfile_.parent_path());

    if ( const Parameters::geometry_STEP_type* geom_cad =
         boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
      {
        using namespace insight::cad;

        FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel);

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

              walls->saveAs(wallstlfile_);
              inlet->saveAs(inletstlfile_);
              outlet->saveAs(outletstlfile_);
          }
        else if ( const Parameters::geometry_STEP_type::inout_extra_files_type* io_extra =
             boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
          {

            cadmodel->saveAs(wallstlfile_);
            FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model);
            inletmodel->saveAs(inletstlfile_);
            FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model);
            outletmodel->saveAs(outletstlfile_);
          }
      }
    else if ( const Parameters::geometry_STL_type* geom_stl = boost::get<Parameters::geometry_STL_type>(&p.geometry) )
      {
        cm.executeCommand(executionPath(), "surfaceConvert", {geom_stl->cadmodel.string(), wallstlfile_.string()});
        cm.executeCommand(executionPath(), "surfaceConvert", {geom_stl->inlet.string(), inletstlfile_.string()});
        cm.executeCommand(executionPath(), "surfaceConvert", {geom_stl->outlet.string(), outletstlfile_.string()});
      }
    
    surfaceFeatureExtract(cm, executionPath(), wallstlfile_.filename().c_str());
    
    snappyHexMeshConfiguration::Parameters shm_cfg;

    arma::mat s = vec3(1,1,1)*p.geometryscale;
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::ExplicitFeatureCurve(snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
      .set_level(p.mesh.maxLevel)
      .set_scale(s)
      .set_fileName(executionPath()/"constant"/"triSurface"/(wallstlfile_.filename().stem().string()+".eMesh"))
    )));

    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("walls")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(p.mesh.nLayers)
      .set_scale(s)

      .set_fileName(wallstlfile_)
    )));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("inlet")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(0)
      .set_scale(s)

      .set_fileName(inletstlfile_)
    )));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("outlet")
      .set_minLevel(p.mesh.minLevel)
      .set_maxLevel(p.mesh.maxLevel)
      .set_nLayers(0)
      .set_scale(s)

      .set_fileName(outletstlfile_)
    )));


    shm_cfg.PiM.push_back(p.mesh.PiM);
    
    snappyHexMesh
    (
      cm, executionPath(),
      shm_cfg
    );


    resetMeshToLatestTimestep(cm, executionPath(), true);
      
    cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite"));

}


void InternalPressureLoss::createCase(insight::OpenFOAMCase& cm)
{
    Parameters p(parameters_);
    
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
    
    insertTurbulenceModel(cm, p);
}


ResultSetPtr InternalPressureLoss::evaluateResults(OpenFOAMCase& cm)
{
    Parameters p(parameters_);
    ResultSetPtr results=insight::OpenFOAMAnalysis::evaluateResults(cm);

    arma::mat p_vs_t = surfaceIntegrate::readSurfaceIntegrate(cm, executionPath(), "inlet_pressure");

    addPlot
    (
      results, executionPath(), "chartPressureDifference",
      "Iteration", "$p/\\rho$",
      list_of<PlotCurve>
          (PlotCurve(p_vs_t.col(0), p_vs_t.col(1), "pmean_vs_iter", "w l not"))
       ,
      "Plot of pressure difference between inlet and outlet vs. iterations"
    );

    double delta_p=p_vs_t(p_vs_t.n_rows-1,1)*p.fluid.rho;
    ptr_map_insert<ScalarResult>(*results) ("delta_p", delta_p, "Pressure difference", "", "Pa");

    {
      double Lmax=p.geometryscale*arma::as_scalar(arma::max(L_));
      arma::mat ctr=p.geometryscale*( bb_.col(1) + bb_.col(0) )*0.5;
//      arma::mat ctri=inletprops.ctr_;

      paraview::ParaviewVisualization::Parameters pvp;
      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::CustomPVScene(paraview::CustomPVScene::Parameters()
        .set_command(
           "import numpy as np\n"

           "eb=extractPatches(openfoam_case, 'wall.*')\n"
           "Show(eb)\n"
           "displaySolid(eb, 0.1)\n"
        )
      )));

      paraview::Streamtracer::Parameters::seed_cloud_type cloud;
      cloud.center=p.mesh.PiM *p.geometryscale;
      cloud.number=500;
      cloud.radius=0.5*arma::norm(L_,2);
      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::Streamtracer(paraview::Streamtracer::Parameters()
        .set_seed(cloud)
        .set_dataset("openfoam_case[0]")
        .set_field("U")
        .set_maxLen(10.*Lmax)
        .set_name("st")
      )));

      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::IsoView(paraview::IsoView::Parameters()
        .set_bbmin(p.geometryscale*bb_.col(0))
        .set_bbmax(p.geometryscale*bb_.col(1))
        .set_filename("streamlines.png")

      )));

      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::CustomPVScene(paraview::CustomPVScene::Parameters()
        .set_command(
            "Hide(st)\n"
            "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.8, 0.25], barorient=1, opacity=1.)\n"
        )
      )));

      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::IsoView(paraview::IsoView::Parameters()
        .set_bbmin(p.geometryscale*bb_.col(0))
        .set_bbmax(p.geometryscale*bb_.col(1))
        .set_filename("pressureContour.png")
      )));

      paraview::ParaviewVisualization pv(pvp, executionPath());
      ResultSetPtr images = pv();
      results->insert ( "renderings", images );
    }
    
    return results;
}


ParameterSet_VisualizerPtr InternalPressureLoss_visualizer()
{
    return ParameterSet_VisualizerPtr( new InternalPressureLoss_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(Analysis, InternalPressureLoss, visualizer, InternalPressureLoss_visualizer);


void InternalPressureLoss_ParameterSet_Visualizer::recreateVisualizationElements(UsageTracker* ut)
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements(ut);

  Parameters p(ps_);

  try
  {

    if ( const Parameters::geometry_STEP_type* geom_cad = boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
      {
        using namespace insight::cad;

        FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel);

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
            FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model);
            addFeature("inlet", inletmodel);
            FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model);
            addFeature("outlet", outletmodel);
          }
      }
    else if ( const Parameters::geometry_STL_type* geom_stl =
              boost::get<Parameters::geometry_STL_type>(&p.geometry) )
      {
        addFeature("walls", insight::cad::STL::create(geom_stl->cadmodel) );
        addFeature("inlet", insight::cad::STL::create(geom_stl->inlet) );
        addFeature("outlet", insight::cad::STL::create(geom_stl->outlet) );
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
