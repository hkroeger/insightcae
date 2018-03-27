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

void InternalPressureLoss::calcDerivedInputData()
{
    insight::OpenFOAMAnalysis::calcDerivedInputData();
    Parameters p(parameters_);
    //reportIntermediateParameter("L", L_, "total domain length", "m");
    
    using namespace insight::cad;
    FeaturePtr cadmodel = Feature::CreateFromFile(p.geometry.cadmodel);
//     cadmodel->build();

    bb_=cadmodel->modelBndBox();
    L_=bb_.col(1)-bb_.col(0);
    nx_=std::max(1, int(ceil(L_(0)/p.mesh.size)));
    ny_=std::max(1, int(ceil(L_(1)/p.mesh.size)));
    nz_=std::max(1, int(ceil(L_(2)/p.mesh.size)));
    reportIntermediateParameter("nx", nx_, "initial grid cell numbers in direction x");
    reportIntermediateParameter("ny", ny_, "initial grid cell numbers in direction y");
    reportIntermediateParameter("nz", nz_, "initial grid cell numbers in direction z");
    
    auto inletss=cadmodel->providedSubshapes().find(p.geometry.inlet_name);
    if (inletss==cadmodel->providedSubshapes().end())
        throw insight::Exception("named face \""+p.geometry.inlet_name+"\" not found in CAD model!");
    inlet_=inletss->second;
    inlet_->checkForBuildDuringAccess();
    
    auto outletss=cadmodel->providedSubshapes().find(p.geometry.outlet_name);
    if (outletss==cadmodel->providedSubshapes().end())
        throw insight::Exception("named face \""+p.geometry.outlet_name+"\" not found in CAD model!");
    outlet_=outletss->second;
    outlet_->checkForBuildDuringAccess();
    
    FeatureSetParserArgList args;
    args.push_back(cadmodel->providedFeatureSet(p.geometry.inlet_name));
    args.push_back(cadmodel->providedFeatureSet(p.geometry.outlet_name));
//     args.push_back(FeatureSetPtr(new FeatureSet(cadmodel, Face, "isPlane && minimal(CoG.x)")));
//     args.push_back(FeatureSetPtr(new FeatureSet(cadmodel, Face, "isPlane && maximal(CoG.x)")));
//     inlet_.reset(new Feature(boost::get<FeatureSetPtr>(args[0])));
//     outlet_.reset(new Feature(boost::get<FeatureSetPtr>(args[1])));
    FeatureSetPtr fp(new FeatureSet(cadmodel, insight::cad::Face, "!( in(%0) || in(%1) )",  args));
    walls_.reset(new Feature(fp));


    inletstlfile_=executionPath()/"constant"/"triSurface"/"inlet.stlb";
    outletstlfile_=executionPath()/"constant"/"triSurface"/"outlet.stlb";
    wallstlfile_=executionPath()/"constant"/"triSurface"/"walls.stlb";

    bbi_=inlet_->modelBndBox();
    arma::mat Li=bbi_.col(1)-bbi_.col(0);
    D_=arma::as_scalar(arma::max(Li)); // not yet the real hydraulic diameter, please improve
    reportIntermediateParameter("D", D_, "hydraulic diameter of inlet", "mm");
    
    Ain_=inlet_->modelSurfaceArea();
    reportIntermediateParameter("Ain", Ain_, "area of inlet", "$mm^2$");
}

void InternalPressureLoss::createMesh(insight::OpenFOAMCase& cm)
{
    Parameters p(parameters_);

    cm.insert(new MeshingNumerics(cm));
    cm.createOnDisk(executionPath());
    
    using namespace insight::bmd;
    std::auto_ptr<blockMesh> bmd(new blockMesh(cm));
    bmd->setScaleFactor(1.0);
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
    
    walls_->saveAs(wallstlfile_);
    inlet_->saveAs(inletstlfile_);
    outlet_->saveAs(outletstlfile_); 

    surfaceFeatureExtract(cm, executionPath(), wallstlfile_.filename().c_str());
    
//     boost::ptr_vector<snappyHexMeshFeats::Feature> shm_feats;
    snappyHexMeshConfiguration::Parameters shm_cfg;

    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::ExplicitFeatureCurve(snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
      .set_level(3)
      .set_fileName(executionPath()/"constant"/"triSurface"/(wallstlfile_.filename().stem().string()+".eMesh"))
    )));

    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("walls")
      .set_minLevel(0)
      .set_maxLevel(3)
      .set_nLayers(2)
      
      .set_fileName(wallstlfile_)
    )));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("inlet")
      .set_minLevel(0)
      .set_maxLevel(3)
      .set_nLayers(0)
      
      .set_fileName(inletstlfile_)
    )));
    shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
      .set_name("outlet")
      .set_minLevel(0)
      .set_maxLevel(3)
      .set_nLayers(0)
      .set_fileName(outletstlfile_)
    )));


    arma::mat bbi=inlet_->modelBndBox();
    std::cout<<"bbi="<<bbi<<std::endl;
    
    arma::mat ctr=0.5*(bbi.col(1)+bbi.col(0));
    ctr(0)+=eps; // some small distance downstream of inlet ctr
    std::cout<<"ctr="<<ctr<<std::endl;
    
    shm_cfg.PiM.push_back(ctr);
    
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
      
    cm.executeCommand(executionPath(), "transformPoints", list_of("-scale")("(1e-3 1e-3 1e-3)") ); // mm => m
    cm.executeCommand(executionPath(), "renumberMesh", list_of("-overwrite"));
}


void InternalPressureLoss::createCase(insight::OpenFOAMCase& cm)
{
    Parameters p(parameters_);
    
    OFDictData::dict boundaryDict;
    cm.parseBoundaryDict(executionPath(), boundaryDict);
    cm.insert(new simpleFoamNumerics(cm));
    cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters() ));
    
    cm.insert(new PressureOutletBC(cm, "outlet", boundaryDict));
    {
        ParameterSet vp = VelocityInletBC::Parameters::makeDefault().merge(
            VelocityInletBC::Parameters()
                .set_velocity( FieldData::uniformSteady(p.operation.Q/(1e-6*Ain_),0,0) )
            );
        vp.get<SelectableSubsetParameter>("turbulence").setSelection
        (
            "uniformIntensityAndLengthScale", 
            turbulenceBC::uniformIntensityAndLengthScale::Parameters()
                .set_I(0.1)
                .set_l(D_*0.2)
        );
        cm.insert(new VelocityInletBC(cm, "inlet", boundaryDict, vp));
    }
        
//       .set_turbulence( uniformIntensityAndLengthScale(0.1, D_*0.2) )
//     ));
    cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );
    
    insertTurbulenceModel(cm, parameters_.get<SelectableSubsetParameter>("fluid/turbulenceModel"));
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
    
    arma::mat pi=patchIntegrate(cm, executionPath(), "p", "inlet", std::vector<std::string>() ); // time, int p, A
    
    arma::mat t=pi.col(0), pmean=pi.col(2)/pi.col(1);
    
    addPlot
    (
      results, executionPath(), "chartPressureDifference",
      "Iteration", "$p/\\rho$",
      list_of<PlotCurve>
       (PlotCurve(t, pmean, "pmean_vs_iter", "w lp not"))
       ,
      "Plot of pressure difference between inlet and outlet vs. iterations"
    );

    double delta_p=pmean(pmean.n_rows-1)*p.fluid.rho;
    ptr_map_insert<ScalarResult>(*results) ("delta_p", delta_p, "Pressure difference", "", "Pa");

    std::string init=
    "cbi=loadOFCase('"+executionPath().string()+"')\n"
    "prepareSnapshots()\n";

    {
      format pvec("[%g, %g, %g]");
      std::string filename="wave_above.png";
      
      double Lmax=1e-3*arma::as_scalar(arma::max(L_));
      arma::mat ctr=1e-3*(bb_.col(1)+bb_.col(0))*0.5;
      arma::mat ctri=1e-3*(bbi_.col(1)+bbi_.col(0))*0.5;
      
      runPvPython
      (
	cm, executionPath(), list_of<std::string>
	(
	  init+
	  "import numpy as np\n"
	  
	  "eb=extractPatches(cbi, 'wall.*')\n"
	  //"fl=extractPatches(cbi, 'floor')\n"
	  "Show(eb)\n"
	  //"Show(fl)\n"
// 	  "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.5, 0.75], barorient=0)\n"
	  "displaySolid(eb, 0.1)\n"
	  
	  "st=StreamTracer(Input=cbi[0], Vectors=['U'], MaximumStreamlineLength="+lexical_cast<string>(10.*Lmax)+")\n"
	  "st.SeedType.Center="+str( pvec % (ctri(0)+1e-3*Lmax) % ctri(1) % ctri(2))+"\n"
	  "st.SeedType.Radius="+lexical_cast<string>(1e-3*0.5*D_)+"\n"
	  "Show(st)\n"

	  "setCam("+str( pvec % bb_(0,0) % bb_(1,0) % bb_(2,1))+", "
	           +str(pvec % ctr(0) % ctr(1) % ctr(2))+", "
		   "[0,0,1], "
		   +str(format("%g") % (/*0.33*1e-3*L_*/ 0.5*Lmax))
		   +")\n"
	  "WriteImage('streamLinesDiag.png')\n"

	  "Hide(st)\n"
	  "displayContour(eb, 'p', arrayType='CELL_DATA', barpos=[0.8, 0.25], barorient=1, opacity=1.)\n"
	  "setCam("+str( pvec % bb_(0,0) % bb_(1,0) % bb_(2,1))+", "
	           +str(pvec % ctr(0) % ctr(1) % ctr(2))+", "
		   "[0,0,1], "
		   +str(format("%g") % (/*0.33*1e-3*L_*/ 0.5*Lmax))
		   +")\n"
	  "WriteImage('pressureContourDiagInlet.png')\n"
	  
	  "setCam("+str( pvec % bb_(0,1) % bb_(1,0) % bb_(2,1))+", "
	           +str(pvec % ctr(0) % ctr(1) % ctr(2))+", "
		   "[0,0,1], "
		   +str(format("%g") % (/*0.33*1e-3*L_*/ 0.5*Lmax))
		   +")\n"
	  "WriteImage('pressureContourDiagOutlet.png')\n"
	  
	  "setCam("+str( pvec % ctr(0) % ctr(1) % bb_(2,1))+", "
	           +str(pvec % ctr(0) % ctr(1) % ctr(2))+", "
		   "[0,1,0], "
		   +str(format("%g") % (/*0.33*1e-3*L_*/ 0.5*Lmax))
		   +")\n"
	  "WriteImage('pressureContourTop.png')\n"
// 	  "setCam("+str( pvec % (-L_) % (-0.5*L_) % (0.33*L_))+
// 		    ", ["+str(format("%g")%(0.5*L_))+",0,0], [0,0,1], "
// 		    +str(format("%g") % (/*0.33*1e-3*L_*/ 1.0))+")\n"
// 	  "WriteImage('leftfrontview.png')\n"
// 
// 	  "setCam("+str( pvec % (2.*L_) % (0.5*L_) % (0.33*L_))+
// 		    ", ["+str(format("%g")%(0.5*L_))+",0,0], [0,0,1], "
// 		    +str(format("%g") % (/*0.33*1e-3*L_*/ 1.0))+")\n"
// 	  "WriteImage('rightrearview.png')\n"
// 	  "setCam("+str( pvec % (2.*L_) % (-0.5*L_) % (0.33*L_))+
// 		    ", ["+str(format("%g")%(0.5*L_))+",0,0], [0,0,1], "
// 		    +str(format("%g") % (/*0.33*1e-3*L_*/ 1.0))+")\n"
// 	  "WriteImage('leftrearview.png')\n"
	)
      );
      results->insert("streamLinesDiag",
	std::auto_ptr<Image>(new Image
	(
	executionPath(), "streamLinesDiag.png", 
	"Flow streamlines", ""
      )));
      results->insert("pressureContourDiagInlet",
	std::auto_ptr<Image>(new Image
	(
	executionPath(), "pressureContourDiagInlet.png", 
	"Pressure contour (view on inlet)", ""
      )));
      results->insert("pressureContourDiagOutlet",
	std::auto_ptr<Image>(new Image
	(
	executionPath(), "pressureContourDiagOutlet.png", 
	"Pressure contour (view on outlet)", ""
      )));
      results->insert("pressureContourTop",
	std::auto_ptr<Image>(new Image
	(
	executionPath(), "pressureContourTop.png", 
	"Pressure contour (top view)", ""
      )));
//       results->insert("contourPressureRightFront",
// 	std::auto_ptr<Image>(new Image
// 	(
// 	executionPath(), "rightfrontview.png", 
// 	"Pressure distribution on car, view from right side ahead", ""
//       )));
//       results->insert("contourPressureLeftRear",
// 	std::auto_ptr<Image>(new Image
// 	(
// 	executionPath(), "leftrearview.png", 
// 	"Pressure distribution on car, view from left side rear", ""
//       )));
//       results->insert("contourPressureRightRear",
// 	std::auto_ptr<Image>(new Image
// 	(
// 	executionPath(), "rightrearview.png", 
// 	"Pressure distribution on car, view from right side rear", ""
//       )));
    }
    
    return results;
}
}
