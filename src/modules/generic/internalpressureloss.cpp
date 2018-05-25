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

#include "cadfeature.h"

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight 
{

addToAnalysisFactoryTable(InternalPressureLoss);

InternalPressureLoss::InternalPressureLoss(const ParameterSet& ps, const boost::filesystem::path& exepath)
: OpenFOAMAnalysis
  (
    "InternalPressureLoss", 
    "Determination of internal pressure loss by CFD a simulation",
    ps, exepath
  )
  // default values for derived parameters
{}

ParameterSet InternalPressureLoss::defaultParameters()
{
    ParameterSet p(OpenFOAMAnalysis::defaultParameters());
    p.merge(Parameters::makeDefault());
    return p;
}


#define extendBB(bb, bb2) \
  for (int i=0; i<3; i++) { \
   bb(i,0)=std::min(bb(i,0), bb2(i,0)); \
   bb(i,1)=std::max(bb(i,1), bb2(i,1)); \
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
    
    if ( const Parameters::geometry_STEP_type* geom_cad = boost::get<Parameters::geometry_STEP_type>(&p.geometry) )
      {
        using namespace insight::cad;

        FeaturePtr cadmodel = Feature::CreateFromFile(geom_cad->cadmodel);
        bb_=cadmodel->modelBndBox();

        if ( const Parameters::geometry_STEP_type::inout_extra_files_type* io_extra =
             boost::get<Parameters::geometry_STEP_type::inout_extra_files_type>(&geom_cad->inout) )
          {
            {
              FeaturePtr inletmodel = Feature::CreateFromFile(io_extra->inlet_model);
              arma::mat bb = inletmodel->modelBndBox();
              extendBB(bb_, bb);
            }

            {
              FeaturePtr outletmodel = Feature::CreateFromFile(io_extra->outlet_model);
              arma::mat bb = outletmodel->modelBndBox();
              extendBB(bb_, bb);
            }
          }
      }
    else if ( const Parameters::geometry_STL_type* geom_stl = boost::get<Parameters::geometry_STL_type>(&p.geometry) )
      {
        bb_ = STLBndBox(geom_stl->cadmodel);
        {
          arma::mat bb = STLBndBox(geom_stl->inlet);
          extendBB(bb_, bb);
        }
        {
          arma::mat bb = STLBndBox(geom_stl->outlet);
          extendBB(bb_, bb);
        }
      }

    L_=bb_.col(1)-bb_.col(0);
    nx_=std::max(1, int(ceil(L_(0)/p.mesh.size)));
    ny_=std::max(1, int(ceil(L_(1)/p.mesh.size)));
    nz_=std::max(1, int(ceil(L_(2)/p.mesh.size)));
    reportIntermediateParameter("nx", nx_, "initial grid cell numbers in direction x");
    reportIntermediateParameter("ny", ny_, "initial grid cell numbers in direction y");
    reportIntermediateParameter("nz", nz_, "initial grid cell numbers in direction z");
    
    
    
//    FeatureSetParserArgList args;
//    args.push_back(cadmodel->providedFeatureSet(p.geometry.inlet_name));
//    args.push_back(cadmodel->providedFeatureSet(p.geometry.outlet_name));
//    FeatureSetPtr fp(new FeatureSet(cadmodel, insight::cad::Face, "!( in(%0) || in(%1) )",  args));
//    walls_.reset(new Feature(fp));



//    bbi_=inlet_->modelBndBox();
//    arma::mat Li=bbi_.col(1)-bbi_.col(0);
//    D_=arma::as_scalar(arma::max(Li)); // not yet the real hydraulic diameter, please improve
//    reportIntermediateParameter("D", D_, "hydraulic diameter of inlet", "mm");
    
//    Ain_=inlet_->modelSurfaceArea();
//    reportIntermediateParameter("Ain", Ain_, "area of inlet", "$mm^2$");
}

void InternalPressureLoss::createMesh(insight::OpenFOAMCase& cm)
{
    Parameters p(parameters_);

    cm.insert(new MeshingNumerics(cm));
    cm.createOnDisk(executionPath());
    
    using namespace insight::bmd;
    std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
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
	Block& bl = bmd->addBlock
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
        copy_file(geom_stl->cadmodel, wallstlfile_, copy_option::overwrite_if_exists);
        copy_file(geom_stl->inlet, inletstlfile_, copy_option::overwrite_if_exists);
        copy_file(geom_stl->outlet, outletstlfile_, copy_option::overwrite_if_exists);
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


//    arma::mat bbi=inlet_->modelBndBox();
//    std::cout<<"bbi="<<bbi<<std::endl;
    
//    arma::mat ctr=0.5*(bbi.col(1)+bbi.col(0));
//    ctr(0)+=eps; // some small distance downstream of inlet ctr
//    std::cout<<"ctr="<<ctr<<std::endl;
    
    shm_cfg.PiM.push_back(p.mesh.PiM);
    
    snappyHexMesh
    (
      cm, executionPath(),
      shm_cfg
//       OFDictData::vector3(ctr),
//       shm_feats,
//       snappyHexMeshOpts::Parameters()
// 	.set_tlayer(1.2)
// 	.set_erlayer(1.3)
	//.set_relativeSizes(false)
    );


    resetMeshToLatestTimestep(cm, executionPath(), true);
      
//    cm.executeCommand(executionPath(), "transformPoints", list_of("-scale")("(1e-3 1e-3 1e-3)") ); // mm => m
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
    cm.insert(new simpleFoamNumerics(cm));
    cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters() ));
    
    cm.insert(new PressureOutletBC(cm, "outlet", boundaryDict));

    {
        MassflowBC::Parameters inp;
        inp.massflow = p.fluid.rho * p.operation.Q;
        inp.turbulence.reset(new turbulenceBC::uniformIntensityAndLengthScale(
                              turbulenceBC::uniformIntensityAndLengthScale::Parameters()
                               .set_I(0.1)
                               .set_l(D*0.2)
        ));
        cm.insert(new MassflowBC(cm, "inlet", boundaryDict, inp));
    }
        
    cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );
    
    insertTurbulenceModel(cm, p);
}


ResultSetPtr InternalPressureLoss::evaluateResults(OpenFOAMCase& cm)
{
    Parameters p(parameters_);
    ResultSetPtr results=insight::OpenFOAMAnalysis::evaluateResults(cm);

//     boost::ptr_vector<sampleOps::set> sets;
//     sets.push_back(new sampleOps::uniformLine(sampleOps::uniformLine::Parameters()
//                    .set_name("axial")
//                    .set_start( vec3(0, 0, 0))
//                    .set_end(   vec3(1, 0, 0))
//                    .set_np(20)
//                                              ));
//     sample(cmp, executionPath(),
//            list_of<std::string>("p")("U"),
//            sets
//           );
//     sampleOps::ColumnDescription cd;
//     arma::mat data = dynamic_cast<sampleOps::uniformLine*>(&sets[0])
//                      ->readSamples(cmp, executionPath(), &cd);
// 
//     {
//         int c=cd["U"].col;
//         arma::mat U(join_rows( data.col(0)/p.geometry.D, data.col(c) ));
//         addPlot
//         (
//             results, executionPath(), "chartMeanVelocity_axial",
//             "x/D", "<U>/U0",
//             list_of
//             (PlotCurve(U, "w l lt 1 lc 1 lw 4 t 'Axial'"))
//             (PlotCurve(refdata_U, "w l lt 2 lc 1 t 'Axial (Exp., J-Mode)'"))
//             ,
//             "Profiles of averaged velocities at centerline"
//         );
//     }
    
    patchArea inletprops(cm, executionPath(), "inlet");
    double D=sqrt(inletprops.A_*4./M_PI);

    patchIntegrate pi(cm, executionPath(), "p", "inlet", std::vector<std::string>() ); // time, int p, A
    
    arma::mat pmean=pi.integral_values_/pi.A_;
    
    addPlot
    (
      results, executionPath(), "chartPressureDifference",
      "Iteration", "$p/\\rho$",
      list_of<PlotCurve>
       (PlotCurve(pi.t_, pmean, "pmean_vs_iter", "w lp not"))
       ,
      "Plot of pressure difference between inlet and outlet vs. iterations"
    );

    double delta_p=pmean(pmean.n_rows-1)*p.fluid.rho;
    ptr_map_insert<ScalarResult>(*results) ("delta_p", delta_p, "Pressure difference", "", "Pa");

    {
      double Lmax=1e-3*arma::as_scalar(arma::max(L_));
      arma::mat ctr=1e-3*(bb_.col(1)+bb_.col(0))*0.5;
      arma::mat ctri=inletprops.ctr_;

      paraview::ParaviewVisualization::Parameters pvp;
      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::CustomPVScene(paraview::CustomPVScene::Parameters()
        .set_command(
           "import numpy as np\n"

           "eb=extractPatches(openfoam_case, 'wall.*')\n"
           "Show(eb)\n"
           "displaySolid(eb, 0.1)\n"

           "st=StreamTracer(Input=openfoam_case[0], Vectors=['U'], MaximumStreamlineLength="+lexical_cast<string>(10.*Lmax)+")\n"
           "st.SeedType.Center="+paraview::PVScene::pvec( ctri + 1e-3*vec3(Lmax,0,0) )+"\n"
           "st.SeedType.Radius="+lexical_cast<string>(0.5*D)+"\n"
           "Show(st)\n"
        )
      )));

      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::IsoView(paraview::IsoView::Parameters()
        .set_bbmin(1e-3*bb_.col(0))
        .set_bbmax(1e-3*bb_.col(1))
        .set_filename("streamlines.png")
      )));

      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::CustomPVScene(paraview::CustomPVScene::Parameters()
        .set_command(
            "Hide(st)\n"
            "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.8, 0.25], barorient=1, opacity=1.)\n"
        )
      )));

      pvp.scenes.push_back(paraview::PVScenePtr(new paraview::IsoView(paraview::IsoView::Parameters()
        .set_bbmin(1e-3*bb_.col(0))
        .set_bbmax(1e-3*bb_.col(1))
        .set_filename("pressureContour.png")
      )));

      paraview::ParaviewVisualization pv(pvp, executionPath());
      ResultSetPtr images = pv();
      results->insert ( "renderings", images );
    }
    
    return results;
}
}
