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

#include "numericalwindtunnel.h"

#include "base/exception.h"
#include "base/units.h"
#include "base/vtktransformation.h"
#include "openfoam/blockmesh.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/snappyhexmesh.h"
#include "openfoam/paraview.h"
#include "openfoam/blockmeshoutputanalyzer.h"


#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"
#include "openfoam/caseelements/basic/singlephasetransportmodel.h"
#include "openfoam/caseelements/boundaryconditions/simplebc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/symmetrybc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"

#include "openfoam/caseelements/analysiscaseelements.h"


#include "cadfeatures.h"
#include "occtools.h"

using namespace arma;
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace boost::filesystem;


namespace insight 
{


defineType(NumericalWindtunnel);
Analysis::Add<NumericalWindtunnel> addNumericalWindtunnel;


void NumericalWindtunnel::modifyDefaults(insight::ParameterSet& p)
{
    p.setBool("run/potentialinit", true);
}


boost::mutex mtx;


NumericalWindtunnel::supplementedInputData::supplementedInputData(
    ParameterSetInput ip,
    const boost::filesystem::path &workDir,
    ProgressDisplayer &parentProgress )
  : supplementedInputDataDerived<Parameters>( ip.forward<Parameters>(), workDir, parentProgress ),
    FOname_allObjects("forcesAllObjects")
{
  CurrentExceptionContext ex("computing further preprocessing informations");

  double bbdefl=0.5;

  double L_upw=arma::norm(p().geometry.transformation.upwarddir,2);
  if (L_upw<1e-12)
    throw insight::Exception("Upward direction vector has zero length!");

  double L_fwd=arma::norm(p().geometry.transformation.forwarddir,2);
  if (L_fwd<1e-12)
    throw insight::Exception("Forward direction vector has zero length!");

  if ( fabs(arma::dot(p().geometry.transformation.upwarddir/L_upw,
                     p().geometry.transformation.forwarddir/L_fwd)-1.) < 1e-12 )
  {
    throw insight::Exception("Upward and forward direction are colinear!");
  }

  cad::is_gp_Trsf rot; rot.SetTransformation
  (
    gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(-1,0,0)), // other way round => wrong
    gp_Ax3(toVec<gp_Pnt>(-p().geometry.transformation.localOrigin),
           toVec<gp_Dir>(p().geometry.transformation.upwarddir),
           toVec<gp_Dir>(p().geometry.transformation.forwarddir) )
  );


  parentProgress.message("Getting geometry file"); // extraction may take place now



  auto toWindTunnelCS =
      SpatialTransformation( p().geometryscale ) // 2. scale
      * rot.toSpatialTransformation(); // 1. rotate

  SpatialTransformation toAttitude(
      vec3Zero(),
      vec3(
          p().geometry.attitude.roll,
          p().geometry.attitude.trim,
          p().geometry.attitude.yaw) );

  parentProgress.message("Loading geometry file, computing bounding box");

  arma::mat bb=arma::zeros(3, 2);// bounding box in SI, rotated to wind tunnel CS
  arma::mat bbAtt=arma::zeros(3, 2);// bounding box in SI, rotated to wind tunnel CS + applied attitude change

  {
      auto loadprogress=parentProgress.forkNewAction(p().geometry.objects.size(), "loading geometry");
      for (auto& g: p().geometry.objects)
      {
          auto geom=cad::Transform::create(g.second->geometry(), toWindTunnelCS);
          bb=unitedBndBox(bb, geom->modelBndBox());

          geom=cad::Transform::create(geom, toAttitude);
          bbAtt=unitedBndBox(bbAtt, geom->modelBndBox());

          geometry_[g.first]=geom;

          loadprogress.stepUp();
      }
  }

  Lref_ = arma::norm( bb.col(1)-bb.col(0), 2);
  reportSupplementQuantity("Lref", Lref_, "Reference length of object", "m");
  if (Lref_ < 1e-12)
  {
      throw insight::Exception("Bounding box of object has zero size!");
  }



  arma::mat pmin=bbAtt.col(0);
  reportSupplementQuantity("pmin", pmin, "Minimum point of bounding box", "m");
  arma::mat pmax=bbAtt.col(1);
  reportSupplementQuantity("pmax", pmax, "Maximum point of bounding box", "m");

  l_=(pmax(0)-pmin(0));
  reportSupplementQuantity("l", l_, "object length", "m");
  if (l_<1e-12)
      throw insight::Exception("Length of the object is zero!");

  w_=(pmax(1)-pmin(1));
  reportSupplementQuantity("w", w_, "object width", "m");
  if (w_<1e-12)
      throw insight::Exception("Width of the object is zero!");

  hup_=pmax(2);
  dlo_=-pmin(2);
  reportSupplementQuantity("hup", hup_, "object height above local origin", "m");
  reportSupplementQuantity("dlo", dlo_, "object extent below local origin", "m");
  if ((hup_+dlo_)<1e-12)
      throw insight::Exception("Height of the object is zero!");


  Lupstream_ = Lref_*p().geometry.LupstreamByL;
  reportSupplementQuantity("Lupstream", Lupstream_, "Domain extent upstream from object", "m");
  Ldownstream_ = Lref_*p().geometry.LdownstreamByL;
  reportSupplementQuantity("Ldownstream", Ldownstream_, "Domain extent downstream from object", "m");
  double Hdom = Lref_*p().geometry.LupByL;
  reportSupplementQuantity("Hdom", Hdom, "Domain height", "m");
  Laside_ = Lref_*p().geometry.LasideByL;
  reportSupplementQuantity("Laside", Laside_, "Domain sideways from object", "m");


  double h;
  if (boost::get<Parameters::geometry_type::verticalPlacement_onFloor_type>(
          &p().geometry.verticalPlacement))
  {
      // no vertical translation
      h=0.;
  }
  else if (boost::get<Parameters::geometry_type::verticalPlacement_onCeiling_type>(
          &p().geometry.verticalPlacement))
  {
      // no vertical translation
      h=Hdom;
  }
  else if (boost::get<Parameters::geometry_type::verticalPlacement_centered_type>(
          &p().geometry.verticalPlacement))
  {
      h=Hdom*0.5;
  }
  else if (auto * ah =
             boost::get<Parameters::geometry_type::verticalPlacement_atHeight_type>(
               &p().geometry.verticalPlacement))
  {
      if (auto *rdh = boost::get<Parameters::geometry_type
                ::verticalPlacement_atHeight_type
                ::height_relativeToDomain_type>(
              &ah->height))
      {
          h=rdh->hByHdomain*Hdom;
      }
      else if (auto *absh = boost::get<Parameters::geometry_type
                                 ::verticalPlacement_atHeight_type
                                 ::height_absolute_type>(
              &ah->height))
      {
          h=absh->h;
      }
      else
          throw insight::UnhandledSelection();
  }
  else
      throw insight::UnhandledSelection();

  h=std::max<double>(0,std::min<double>(Hdom,h));

  Lup_=std::max(0., Hdom-h-hup_);
  reportSupplementQuantity("Lup", Lup_, "Domain height above lower bound of object", "m");

  Ldown_=std::max(0., h-dlo_);
  reportSupplementQuantity("Ldown", Ldown_, "Domain height below object", "m");



  SpatialTransformation windTunnelPlacement(
      vec3(-pmin(0),-0.5*(pmin(1)+pmax(1)),h)
      );

  // apply to geometry
  for (auto& g: geometry_)
  {
      g.second=cad::Transform::create(g.second, windTunnelPlacement);
  }

  double dx=Lref_/double(p().mesh.nx);

  int nx=std::max(1, int(l_/dx));
  int ny=std::max(1, int(w_/dx));
  int nz=std::max(1, int((hup_+dlo_)/dx));

  int n_upstream=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_upstream).calc_n(dx, Lupstream_));
  int n_downstream=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_downstream).calc_n(dx, Ldownstream_));
  int n_up=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_up).calc_n(dx, Lup_));
  int n_down=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_up).calc_n(dx, Ldown_));
  int n_aside=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_aside).calc_n(dx, Laside_-0.5*w_));


  using namespace insight::bmd;
  blocking=std::make_shared<blockMeshBlocking>();

  blocking->setScaleFactor(1.0);
  blocking->setDefaultPatch("walls", "patch");


  Patch& inlet = 	blocking->addPatch("inlet", new Patch());
  Patch& outlet = 	blocking->addPatch("outlet", new Patch());
  Patch& side1 = 	blocking->addPatch("side1", new Patch());
  Patch& side2 = 	blocking->addPatch("side2", new Patch());
  Patch& top = 		blocking->addPatch("top", new Patch("symmetryPlane"));
  Patch& floor = 	blocking->addPatch("floor", new Patch("wall"));

  // points in cross section
  std::map<int, bmd::Point> pts = {
      {100, 	vec3( -Lupstream_, 0, 0)},
      {101, 	vec3( 0, 0, 0)},
      {102, 	vec3( l_, 0, 0)},
      {103, 	vec3( l_+Ldownstream_, 0, 0)},

      {0, 	vec3( -Lupstream_, 0, Ldown_)},
      {1, 	vec3( 0, 0, Ldown_)},
      {2, 	vec3( l_, 0, Ldown_)},
      {3, 	vec3( l_+Ldownstream_, 0,Ldown_)},

      {4, 	vec3( -Lupstream_, 0, Hdom-Lup_)},
      {5, 	vec3( 0, 0, Hdom-Lup_)},
      {6, 	vec3( l_, 0, Hdom-Lup_)},
      {7, 	vec3( l_+Ldownstream_, 0, Hdom-Lup_)},

      {8, 	vec3( -Lupstream_, 0, Hdom)},
      {9, 	vec3( 0, 0, Hdom)},
      {10, 	vec3( l_, 0, Hdom)},
      {11, 	vec3( l_+Ldownstream_, 0, Hdom)}
  };
  arma::mat Lv=vec3(0,1,0);

  std::vector<int> nzs;
  std::vector<double> grads;
  std::vector<arma::mat> y0;

  if (p().mesh.longitudinalSymmetry)
  {
      nzs= {n_aside, std::max(1,ny/2)};
      grads = {1./p().mesh.grad_aside, 1};
      y0 = {vec3(0,Laside_,0), vec3(0,0.5*w_,0), vec3(0,0,0)};
  }
  else
  {
      nzs= {n_aside, ny, n_aside};
      grads = {1./p().mesh.grad_aside, 1, p().mesh.grad_aside};
      y0 = {vec3(0,Laside_,0), vec3(0,0.5*w_,0), vec3(0,-0.5*w_,0), vec3(0,-Laside_,0)};
  }

  for (size_t i=0; i<nzs.size(); i++)
  {
      if (Ldown_>SMALL)
      {
          {
              Block& bl = blocking->addBlock
                          (
                              new Block(P_8(
                                            pts[100]+y0[i], pts[101]+y0[i], pts[1]+y0[i], pts[0]+y0[i],
                                            pts[100]+y0[i+1], pts[101]+y0[i+1], pts[1]+y0[i+1], pts[0]+y0[i+1]
                                            ),
                                        n_upstream, n_down, nzs[i],
                                        { 1./p().mesh.grad_upstream, 1./p().mesh.grad_up, grads[i] }
                                        )
                              );

              inlet.addFace(bl.face("0473"));
              floor.addFace(bl.face("0154"));
              if (i==0) side1.addFace(bl.face("0321"));
              if (i==2) side2.addFace(bl.face("4567"));
          }
          {
              Block& bl = blocking->addBlock
                          (
                              new Block(P_8(
                                            pts[101]+y0[i], pts[102]+y0[i], pts[2]+y0[i], pts[1]+y0[i],
                                            pts[101]+y0[i+1], pts[102]+y0[i+1], pts[2]+y0[i+1], pts[1]+y0[i+1]
                                            ),
                                        nx, n_down, nzs[i],
                                        { 1., 1./p().mesh.grad_up, grads[i] }
                                        )
                              );
              floor.addFace(bl.face("0154"));
              if (i==0) side1.addFace(bl.face("0321"));
              if (i==2) side2.addFace(bl.face("4567"));
          }
          {
              Block& bl = blocking->addBlock
                          (
                              new Block(P_8(
                                            pts[102]+y0[i], pts[103]+y0[i], pts[3]+y0[i], pts[2]+y0[i],
                                            pts[102]+y0[i+1], pts[103]+y0[i+1], pts[3]+y0[i+1], pts[2]+y0[i+1]
                                            ),
                                        n_downstream, n_down, nzs[i],
                                        { p().mesh.grad_downstream, 1./p().mesh.grad_up, grads[i] }
                                        )
                              );
              outlet.addFace(bl.face("1265"));
              floor.addFace(bl.face("0154"));
              if (i==0) side1.addFace(bl.face("0321"));
              if (i==2) side2.addFace(bl.face("4567"));
          }
      }

      {
          Block& bl = blocking->addBlock
                      (
                          new Block(P_8(
                                        pts[0]+y0[i], pts[1]+y0[i], pts[5]+y0[i], pts[4]+y0[i],
                                        pts[0]+y0[i+1], pts[1]+y0[i+1], pts[5]+y0[i+1], pts[4]+y0[i+1]
                                        ),
                                    n_upstream, nz, nzs[i],
                                    { 1./p().mesh.grad_upstream, 1., grads[i] }
                                    )
                          );

          inlet.addFace(bl.face("0473"));
          if (!(Ldown_>0.)) floor.addFace(bl.face("0154"));
          if (!(Lup_>0)) top.addFace(bl.face("2376"));
          if (i==0) side1.addFace(bl.face("0321"));
          if (i==2) side2.addFace(bl.face("4567"));
      }
      {
          Block& bl = blocking->addBlock
                      (
                          new Block(P_8(
                                        pts[1]+y0[i], pts[2]+y0[i], pts[6]+y0[i], pts[5]+y0[i],
                                        pts[1]+y0[i+1], pts[2]+y0[i+1], pts[6]+y0[i+1], pts[5]+y0[i+1]
                                        ),
                                    nx, nz, nzs[i],
                                    { 1., 1., grads[i] }
                                    )
                          );
          if (!(Ldown_>0.)) floor.addFace(bl.face("0154"));
          if (!(Lup_>0)) top.addFace(bl.face("2376"));
          if (i==0) side1.addFace(bl.face("0321"));
          if (i==2) side2.addFace(bl.face("4567"));
      }
      {
          Block& bl = blocking->addBlock
                      (
                          new Block(P_8(
                                        pts[2]+y0[i], pts[3]+y0[i], pts[7]+y0[i], pts[6]+y0[i],
                                        pts[2]+y0[i+1], pts[3]+y0[i+1], pts[7]+y0[i+1], pts[6]+y0[i+1]
                                        ),
                                    n_downstream, nz, nzs[i],
                                    { p().mesh.grad_downstream, 1., grads[i] }
                                    )
                          );
          outlet.addFace(bl.face("1265"));
          if (!(Ldown_>0.)) floor.addFace(bl.face("0154"));
          if (!(Lup_>0)) top.addFace(bl.face("2376"));
          if (i==0) side1.addFace(bl.face("0321"));
          if (i==2) side2.addFace(bl.face("4567"));
      }

      if (Lup_>SMALL)
          {
          {
              Block& bl = blocking->addBlock
                          (
                              new Block(P_8(
                                            pts[4]+y0[i], pts[5]+y0[i], pts[9]+y0[i], pts[8]+y0[i],
                                            pts[4]+y0[i+1], pts[5]+y0[i+1], pts[9]+y0[i+1], pts[8]+y0[i+1]
                                            ),
                                        n_upstream, n_up, nzs[i],
                                        { 1./p().mesh.grad_upstream, p().mesh.grad_up, grads[i] }
                                        )
                              );
              inlet.addFace(bl.face("0473"));
              top.addFace(bl.face("2376"));
              if (i==0) side1.addFace(bl.face("0321"));
              if (i==2) side2.addFace(bl.face("4567"));
          }
          {
              Block& bl = blocking->addBlock
                          (
                              new Block(P_8(
                                            pts[5]+y0[i], pts[6]+y0[i], pts[10]+y0[i], pts[9]+y0[i],
                                            pts[5]+y0[i+1], pts[6]+y0[i+1], pts[10]+y0[i+1], pts[9]+y0[i+1]
                                            ),
                                        nx, n_up, nzs[i],
                                        { 1, p().mesh.grad_up, grads[i] }
                                        )
                              );
              top.addFace(bl.face("2376"));
              if (i==0) side1.addFace(bl.face("0321"));
              if (i==2) side2.addFace(bl.face("4567"));
          }
          {
              Block& bl = blocking->addBlock
                          (
                              new Block(P_8(
                                            pts[6]+y0[i], pts[7]+y0[i], pts[11]+y0[i], pts[10]+y0[i],
                                            pts[6]+y0[i+1], pts[7]+y0[i+1], pts[11]+y0[i+1], pts[10]+y0[i+1]
                                            ),
                                        n_downstream, n_up, nzs[i],
                                        { p().mesh.grad_downstream, p().mesh.grad_up, grads[i] }
                                        )
                              );
              outlet.addFace(bl.face("1265"));
              top.addFace(bl.face("2376"));
              if (i==0) side1.addFace(bl.face("0321"));
              if (i==2) side2.addFace(bl.face("4567"));
          }
      }
  }
}




NumericalWindtunnel::NumericalWindtunnel(
    const std::shared_ptr<supplementedInputDataBase>& sp )
: OpenFOAMAnalysis(sp)
{}



void NumericalWindtunnel::calcDerivedInputData(ProgressDisplayer& parentProgresss)
{
  reportIntermediateParameter("Lref", sp().Lref_, "reference length of object (bounding box diagonal)", "m");
}




void NumericalWindtunnel::createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentProgress)
{
  path dir = executionPath();
  
  // boost::filesystem::path objectSTLFile =
  //  snappyHexMeshFeats::geometryDir(cm, executionPath())/
  //  (p().geometry.objectfile->fileNameStem()+".stlb");

  cm.insert(new MeshingNumerics(cm, MeshingNumerics::Parameters()
    .set_np(np())
  ));
  cm.createOnDisk(executionPath());
  
  int nb = cm.insert(new bmd::blockMesh(cm, *sp().blocking))->nBlocks();
  
  cm.createOnDisk(executionPath());
  cm.runBlockMesh(executionPath(), nb, &parentProgress);
    
  // create_directory(objectSTLFile.parent_path());

  // std::string geom_file_ext = p().geometry.objectfile->fileExtension();

  // if (geom_file_ext==".stl" || geom_file_ext==".stlb")
  // {
  //   vtk_ChangeCS trsf(
  //         std::bind(&gp_Trsf::Value, &sp().cad_to_cfd_, std::placeholders::_1, std::placeholders::_2),
  //         3, 1
  //        );
  //   writeSTL( readSTL(p().geometry.objectfile->accessibleFilePath(), { &trsf }), objectSTLFile );
  // }
  // else
  // {
  //   boost::mutex::scoped_lock lock(mtx);

  //   auto obj = cad::Transform::create(
  //         cad::Import::create(p().geometry.objectfile->accessibleFilePath()),
  //         sp().cad_to_cfd_
  //         );
  //   obj->exportSTL(objectSTLFile);
  // }


  snappyHexMeshConfiguration::Parameters shm_cfg;
  
  for (auto& g: sp().geometry_)
  {
      shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
          new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
        .set_minLevel(p().mesh.lmsurf)
        .set_maxLevel(p().mesh.lxsurf)
        .set_nLayers(p().mesh.nlayer)
        .set_geometry(make_geometryFile(g.second))
        .set_name(g.first)
      )));
  }
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
   new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
    .set_min(vec3(-0.33*sp().l_, -(0.5+0.5)*sp().w_, sp().Ldown_))
    .set_max(vec3(2.0*sp().l_, (0.5+0.5)*sp().w_, sp().Ldown_+1.33*(sp().hup_+sp().dlo_)))
    
    .set_level(p().mesh.boxlevel)
    .set_name("refinement_box")
  )));
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
    .set_min(vec3(0.8*sp().l_, -(0.5+0.25)*sp().w_, sp().Ldown_))
    .set_max(vec3(1.5*sp().l_, (0.5+0.25)*sp().w_, sp().Ldown_+1.2*(sp().hup_+sp().dlo_)))
    
    .set_level(p().mesh.rearlevel)
    .set_name("refinement_rear")
  )));


  int iref=0;
  for (const Parameters::mesh_type::refinementZones_default_type& rz:
       p().mesh.refinementZones)
  {
    if (const auto* bc =
        boost::get<Parameters::mesh_type::refinementZones_default_type::geometry_box_centered_type>(&rz.geometry))
    {
      arma::mat d = vec3(bc->L, bc->W, bc->H);
      shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
        .set_min(bc->pc - 0.5*d)
        .set_max(bc->pc + 0.5*d)

        .set_level(rz.lx)
        .set_name(str(format("refinement_%d")%iref))
      )));
    }
    else if (const auto* b =
        boost::get<Parameters::mesh_type::refinementZones_default_type::geometry_box_type>(&rz.geometry))
    {
      shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
        .set_min(b->pmin)
        .set_max(b->pmax)

        .set_level(rz.lx)
        .set_name(str(format("refinement_%d")%iref))
      )));
    }

    iref++;
  }

  
  shm_cfg.PiM.push_back(vec3(-0.999*sp().Lupstream_,1e-6,1e-6));

  shm_cfg
  .set_tlayer ( p().mesh.tlayer )
  .set_erlayer ( 1.3 )
  ;
  
  snappyHexMesh
  (
    cm, executionPath(),
    shm_cfg,
    true, false, false,
    &parentProgress
  );

  
  resetMeshToLatestTimestep(cm, executionPath(), true);
  
}



void NumericalWindtunnel::createCase(insight::OpenFOAMCase& cm, ProgressDisplayer&)
{
  //double vside=tan(yaw*M_PI/180.)*p.operation.v;

  double turbI=0.01; // Free-stream turbulence
  double turbL=0.001*(sp().hup_+sp().dlo_); // Free-stream turbulence length scale => very low 0.1% of car height

  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new steadyIncompressibleNumerics(cm, steadyIncompressibleNumerics::Parameters()
    .set_writeInterval(100.0)
    .set_purgeWrite(0)
    .set_endTime(1000.0)
    .set_deltaT(1)
  ));

  forces::Parameters::patches_type patchList;
  for (auto& g: sp().geometry_)
  {
      std::string ppat="\""+g.first+".*\"";
      patchList.push_back(ppat);

      if (sp().geometry_.size()>1)
      {
          cm.insert(new forces(
              cm, forces::Parameters()
                  .set_rhoInf(p().fluid.rho)
                  .set_patches({ ppat })
                  .set_name("forces_"+g.first)
              ));
      }
  }
  cm.insert(new forces(cm, forces::Parameters()
    .set_rhoInf(p().fluid.rho)
    .set_patches(patchList)
    .set_name(sp().FOname_allObjects)
  ));

  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters()
    .set_nu(p().fluid.nu)
  ));

  {
    cm.insert(new PressureOutletBC(cm, "side1", boundaryDict));
  }

  if (p().mesh.longitudinalSymmetry)
  {
    cm.insert(new SymmetryBC(cm, "side2", boundaryDict));
  }
  else
  {
    cm.insert(new PressureOutletBC(cm, "side2", boundaryDict));
  }

  auto *pl=boost::get<Parameters::geometry_type::verticalPlacement_onFloor_type>(
      &p().geometry.verticalPlacement);
  if (pl && pl->wallIsMoving)
  {
      cm.insert(new WallBC(cm, "floor", boundaryDict, WallBC::Parameters()
        .set_wallVelocity(vec3(p().operation.v,0,0)) // velocity of car vs ground! (wind excluded)
      ));
  }
  else
  {
      cm.insert(new SimpleBC(cm, "floor", boundaryDict, "symmetryPlane"));
  }

  auto *pt=boost::get<Parameters::geometry_type::verticalPlacement_onCeiling_type>(
      &p().geometry.verticalPlacement);
  if (pt && pt->wallIsMoving)
  {
      cm.insert(new WallBC(cm, "top", boundaryDict, WallBC::Parameters()
                               .set_wallVelocity(vec3(p().operation.v,0,0))
                           ));
  }
  else
  {
      cm.insert(new SimpleBC(cm, "top", boundaryDict, "symmetryPlane"));
  }


  cm.insert(new PressureOutletBC(cm, "outlet", boundaryDict));
  cm.insert(new VelocityInletBC(cm, "inlet", boundaryDict, VelocityInletBC::Parameters()
      .set_velocity( FieldData::uniformSteady(p().operation.v,0,0) )
      .set_turbulence(turbulenceBC::turbulenceBCPtr(new turbulenceBC::uniformIntensityAndLengthScale(
           turbulenceBC::uniformIntensityAndLengthScale::Parameters()
            .set_I(turbI)
            .set_l(turbL)
        )))
  ));

  cm.addRemainingBCs<WallBC>(boundaryDict, WallBC::Parameters() );

  insertTurbulenceModel(cm, p().fluid.turbulenceModel);

}





    
}
