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

#include "base/vtkrendering.h"
#include "vtkDataSetMapper.h"
#include "vtkStreamTracer.h"
#include "vtkPointSource.h"
#include "vtkPolyDataMapper.h"

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
    FOname("forces")
{
  CurrentExceptionContext ex("computing further preprocessing informations");

  double bbdefl=0.5;

  double L_upw=arma::norm(p().geometry.upwarddir,2);
  if (L_upw<1e-12)
    throw insight::Exception("Upward direction vector has zero length!");

  double L_fwd=arma::norm(p().geometry.forwarddir,2);
  if (L_fwd<1e-12)
    throw insight::Exception("Forward direction vector has zero length!");

  if ( fabs(arma::dot(p().geometry.upwarddir/L_upw, p().geometry.forwarddir/L_fwd)-1.) < 1e-12 )
  {
    throw insight::Exception("Upward and forward direction are colinear!");
  }

  gp_Trsf rot; rot.SetTransformation
  (
    gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(-1,0,0)), // other way round => wrong
    gp_Ax3(gp_Pnt(0,0,0),
           toVec<gp_Dir>(p().geometry.upwarddir),
           toVec<gp_Dir>(p().geometry.forwarddir) )
  );

  arma::mat
      bb, // bounding box in SI, rotated to wind tunnel CS
      bbAtt; // bounding box in SI, rotated to wind tunnel CS + applied attitude change

  parentProgress.message("Getting geometry file"); // extraction may take place now
  std::string geom_file_ext = p().geometry.objectfile->fileName().extension().string();
  boost::to_lower(geom_file_ext);


  gp_Trsf roll; roll.SetRotation(
      gp::OX(),
      toValue(
          p().geometry.attitude.roll*si::angle_deg,
          si::radians));
  gp_Trsf trim; trim.SetRotation(
      gp::OY(),
      toValue(
          p().geometry.attitude.trim*si::angle_deg,
          si::radians));
  gp_Trsf yaw; yaw.SetRotation(
      gp::OZ(),
      toValue(
          p().geometry.attitude.yaw*si::angle_deg,
          si::radians));
  gp_Trsf att=yaw
            .Multiplied(trim)
            .Multiplied(roll);

  parentProgress.message("Loading geometry file, computing bounding box");
  if (geom_file_ext==".stl" || geom_file_ext==".stlb")
  {
    vtk_ChangeCS trsf(
          std::bind(&gp_Trsf::Value, &rot, std::placeholders::_1, std::placeholders::_2),
          3, 1
         );

    auto geopositioned=readSTL(p().geometry.objectfile->filePath(), { &trsf });
    bb = p().geometryscale
         * STLBndBox(geopositioned);

    auto geofinal = cad::OCCtransformToOF(att)
                        .apply_VTK_Transform(geopositioned);
    bbAtt = p().geometryscale
            * STLBndBox( geofinal );
  }
  else
  {
    boost::mutex::scoped_lock lock(mtx);

    auto geopositioned = cad::Transform::create(
          cad::Import::create(p().geometry.objectfile->filePath()), rot);
    bb = p().geometryscale
         * geopositioned->modelBndBox(bbdefl);

    auto geofinal = cad::Transform::create(
        geopositioned, att);
    bbAtt = p().geometryscale
         * geofinal->modelBndBox(bbdefl);
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

  h_=(pmax(2)-pmin(2));
  reportSupplementQuantity("h", h_, "object height", "m");
  if (h_<1e-12)
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
  gp_Vec deltaVert(0, 0, h);

  Lup_=Hdom-h;
  reportSupplementQuantity("Lup", Lup_, "Domain height above lower bound of object", "m");

  Ldown_=h;
  reportSupplementQuantity("Ldown", Ldown_, "Domain height below object", "m");

  gp_Trsf sc; sc.SetScaleFactor(p().geometryscale);
  gp_Trsf tr; tr.SetTranslation(gp_Vec(-pmin(0), -0.5*(pmax(1)+pmin(1)), -pmin(2)));
  gp_Trsf mv; mv.SetTranslation(deltaVert);

  cad_to_cfd_ = mv
        .Multiplied(tr)
        .Multiplied(sc)
        .Multiplied(att)
        // .Multiplied(yaw)
        // .Multiplied(trim)
        // .Multiplied(roll)
        .Multiplied(rot);


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
  
  boost::filesystem::path objectSTLFile =
   snappyHexMeshFeats::geometryDir(cm, executionPath())/
   (p().geometry.objectfile->fileName().stem().string()+".stlb");

  cm.insert(new MeshingNumerics(cm, MeshingNumerics::Parameters()
    .set_np(np())
  ));
  cm.createOnDisk(executionPath());
  
  
  double dx=sp().Lref_/double(p().mesh.nx);

  int nx=std::max(1, int(sp().l_/dx));
  int ny=std::max(1, int(sp().w_/dx));
  int nz=std::max(1, int(sp().h_/dx));

  int n_upstream=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_upstream).calc_n(dx, sp().Lupstream_));
  int n_downstream=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_downstream).calc_n(dx, sp().Ldownstream_));
  int n_up=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_up).calc_n(dx, sp().Lup_-sp().h_));
  int n_down=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_up).calc_n(dx, sp().Ldown_-sp().h_));
  int n_aside=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_aside).calc_n(dx, sp().Laside_-0.5*sp().w_));
  

  using namespace insight::bmd;
  std::unique_ptr<blockMesh> bmd(new blockMesh(cm));
  bmd->setScaleFactor(1.0);
  bmd->setDefaultPatch("walls", "patch");


  Patch& inlet = 	bmd->addPatch("inlet", new Patch());
  Patch& outlet = 	bmd->addPatch("outlet", new Patch());
  Patch& side1 = 	bmd->addPatch("side1", new Patch());
  Patch& side2 = 	bmd->addPatch("side2", new Patch());
  Patch& top = 		bmd->addPatch("top", new Patch("symmetryPlane"));
  Patch& floor = 	bmd->addPatch("floor", new Patch("wall"));

  // points in cross section
  std::map<int, bmd::Point> pts = {
        {100, 	vec3( -sp().Lupstream_, 0, 0)},
        {101, 	vec3( 0, 0, 0)},
        {102, 	vec3( sp().l_, 0, 0)},
        {103, 	vec3( sp().l_+sp().Ldownstream_, 0, 0)},

        {0, 	vec3( -sp().Lupstream_, 0, sp().Ldown_)},
        {1, 	vec3( 0, 0, sp().Ldown_)},
        {2, 	vec3( sp().l_, 0, sp().Ldown_)},
        {3, 	vec3( sp().l_+sp().Ldownstream_, 0, sp().Ldown_)},

        {4, 	vec3( -sp().Lupstream_, 0, sp().Ldown_+sp().h_)},
        {5, 	vec3( 0, 0, sp().Ldown_+sp().h_)},
        {6, 	vec3( sp().l_, 0, sp().Ldown_+sp().h_)},
        {7, 	vec3( sp().l_+sp().Ldownstream_, 0, sp().Ldown_+sp().h_)},

        {8, 	vec3( -sp().Lupstream_, 0, sp().Ldown_+sp().Lup_)},
        {9, 	vec3( 0, 0, sp().Ldown_+sp().Lup_)},
        {10, 	vec3( sp().l_, 0, sp().Ldown_+sp().Lup_)},
        {11, 	vec3( sp().l_+sp().Ldownstream_, 0, sp().Ldown_+sp().Lup_)}
  };
  arma::mat Lv=vec3(0,1,0);

  std::vector<int> nzs;
  std::vector<double> grads;
  std::vector<arma::mat> y0;

  if (p().mesh.longitudinalSymmetry)
  {
    nzs= {n_aside, std::max(1,ny/2)};
    grads = {1./p().mesh.grad_aside, 1};
    y0 = {vec3(0,sp().Laside_,0), vec3(0,0.5*sp().w_,0), vec3(0,0,0)};
  }
  else
  {
    nzs= {n_aside, ny, n_aside};
    grads = {1./p().mesh.grad_aside, 1, p().mesh.grad_aside};
    y0 = {vec3(0,sp().Laside_,0), vec3(0,0.5*sp().w_,0), vec3(0,-0.5*sp().w_,0), vec3(0,-sp().Laside_,0)};
  }
  
  for (size_t i=0; i<nzs.size(); i++)
  {
      if (sp().Ldown_>0.)
      {
          {
              Block& bl = bmd->addBlock
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
              Block& bl = bmd->addBlock
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
              Block& bl = bmd->addBlock
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
          Block& bl = bmd->addBlock
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
          if (!(sp().Ldown_>0.)) floor.addFace(bl.face("0154"));
          if (i==0) side1.addFace(bl.face("0321"));
          if (i==2) side2.addFace(bl.face("4567"));
    }
    {
        Block& bl = bmd->addBlock
                    (
                        new Block(P_8(
                                      pts[1]+y0[i], pts[2]+y0[i], pts[6]+y0[i], pts[5]+y0[i],
                                      pts[1]+y0[i+1], pts[2]+y0[i+1], pts[6]+y0[i+1], pts[5]+y0[i+1]
                                      ),
                                  nx, nz, nzs[i],
                                  { 1., 1., grads[i] }
                                  )
                        );
        if (!(sp().Ldown_>0.)) floor.addFace(bl.face("0154"));
        if (i==0) side1.addFace(bl.face("0321"));
        if (i==2) side2.addFace(bl.face("4567"));
    }
    {
        Block& bl = bmd->addBlock
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
        if (!(sp().Ldown_>0.)) floor.addFace(bl.face("0154"));
        if (i==0) side1.addFace(bl.face("0321"));
        if (i==2) side2.addFace(bl.face("4567"));
    }

    {
        Block& bl = bmd->addBlock
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
        Block& bl = bmd->addBlock
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
        Block& bl = bmd->addBlock
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
  
  int nb=bmd->nBlocks();
  cm.insert(bmd.release());
  
  cm.createOnDisk(executionPath());
  cm.runBlockMesh(executionPath(), nb, &parentProgress);
    
  create_directory(objectSTLFile.parent_path());

  std::string geom_file_ext = p().geometry.objectfile->fileName().extension().string();
  boost::to_lower(geom_file_ext);

  if (geom_file_ext==".stl" || geom_file_ext==".stlb")
  {
    vtk_ChangeCS trsf(
          std::bind(&gp_Trsf::Value, &sp().cad_to_cfd_, std::placeholders::_1, std::placeholders::_2),
          3, 1
         );
    writeSTL( readSTL(p().geometry.objectfile->filePath(), { &trsf }), objectSTLFile );
  }
  else
  {
    boost::mutex::scoped_lock lock(mtx);

    auto obj = cad::Transform::create(
          cad::Import::create(p().geometry.objectfile->filePath()),
          sp().cad_to_cfd_
          );
    obj->exportSTL(objectSTLFile);
  }


  snappyHexMeshConfiguration::Parameters shm_cfg;
  
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
      new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
    .set_minLevel(p().mesh.lmsurf)
    .set_maxLevel(p().mesh.lxsurf)
    .set_nLayers(p().mesh.nlayer)
    .set_fileName(make_filepath(objectSTLFile))
    .set_name("object")
  )));
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
   new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
    .set_min(vec3(-0.33*sp().l_, -(0.5+0.5)*sp().w_, sp().Ldown_))
    .set_max(vec3(2.0*sp().l_, (0.5+0.5)*sp().w_, sp().Ldown_+1.33*sp().h_))
    
    .set_level(p().mesh.boxlevel)
    .set_name("refinement_box")
  )));
  shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(new snappyHexMeshFeats::RefinementBox(snappyHexMeshFeats::RefinementBox::Parameters()
    .set_min(vec3(0.8*sp().l_, -(0.5+0.25)*sp().w_, sp().Ldown_))
    .set_max(vec3(1.5*sp().l_, (0.5+0.25)*sp().w_, sp().Ldown_+1.2*sp().h_))
    
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
  double turbL=0.001*sp().h_; // Free-stream turbulence length scale => very low 0.1% of car height

  path dir = executionPath();

  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(dir, boundaryDict);

  cm.insert(new steadyIncompressibleNumerics(cm, steadyIncompressibleNumerics::Parameters()
    .set_writeInterval(100.0)
    .set_purgeWrite(0)
    .set_endTime(1000.0)
    .set_deltaT(1)
  ));
  cm.insert(new forces(cm, forces::Parameters()
    .set_rhoInf(p().fluid.rho)
    .set_patches({ "\"(object.*)\"" })
    .set_name(sp().FOname)
  ));
  cm.insert(new singlePhaseTransportProperties(cm, singlePhaseTransportProperties::Parameters()
    .set_nu(p().fluid.nu)
  ));

  cm.insert(new SimpleBC(cm, "top", boundaryDict, "symmetryPlane"));

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

  if (p().operation.lowerWallIsMoving)
  {
      cm.insert(new WallBC(cm, "floor", boundaryDict, WallBC::Parameters()
        .set_wallVelocity(vec3(p().operation.v,0,0)) // velocity of car vs ground! (wind excluded)
      ));
  }
  else
  {
      cm.insert(new SimpleBC(cm, "floor", boundaryDict, "symmetryPlane"));
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




ResultSetPtr NumericalWindtunnel::evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& pp)
{
  ResultSetPtr results=insight::OpenFOAMAnalysis::evaluateResults(cm, pp);

  auto ap = pp.forkNewAction(13, "Evaluation");
  
  // get full name of car patch (depends on STL file)
  OFDictData::dict boundaryDict;
  cm.parseBoundaryDict(executionPath(), boundaryDict);
  std::string carPatchName;
  for (const OFDictData::dict::value_type& de: boundaryDict)
  {
    if (starts_with(de.first, "object"))
    {
      carPatchName=de.first;
      break;
    }
  }

  ap.message("Computing projected area");
  arma::mat Ah=projectedArea(cm, executionPath(), 
    vec3(1,0,0),
    {"object"}
  );
  double A=Ah(Ah.n_rows-1,1);
++ap;

  
  double Re=p().operation.v*sp().Lref_/p().fluid.nu;
  ptr_map_insert<ScalarResult>(*results) ("Re", Re, "Reynolds number", "", "");
  ptr_map_insert<ScalarResult>(*results) ("Afront", A, "Projected frontal area", "", "$m^2$");
    
  ap.message("Reading forces");
  arma::mat f=forces::readForces(cm, executionPath(), sp().FOname);
  arma::mat t = f.col(0);
++ap;
  
  double mult = p().mesh.longitudinalSymmetry ? 2.0 : 1.0;

  arma::mat Rtot = (f.col(1)+f.col(4)) *mult;
  arma::mat Flat = (f.col(2)+f.col(5)) *mult;
  arma::mat L = (f.col(3)+f.col(6)) *mult;
  
  ptr_map_insert<ScalarResult>(*results) ("Rtot", Rtot(Rtot.n_rows-1), "Total resistance", "", "N");
  ptr_map_insert<ScalarResult>(*results) ("Flat", Flat(Flat.n_rows-1), "Lateral force", "", "N");
  ptr_map_insert<ScalarResult>(*results) ("L", L(L.n_rows-1), "Lifting force", "", "N");

  double cr=Rtot(Rtot.n_rows-1) / (0.5*p().fluid.rho*pow(p().operation.v,2)*A);
  ptr_map_insert<ScalarResult>(*results) ("cr", cr, "Resistance coefficient", "", "");
  
  double cl=L(L.n_rows-1) / (0.5*p().fluid.rho*pow(p().operation.v,2)*A);
  ptr_map_insert<ScalarResult>(*results) ("cl", cl, "Lifting coefficient", "with respect to projected frontal area and forward velocity", "");

  double cs=Flat(Flat.n_rows-1) / (0.5*p().fluid.rho*pow(p().operation.v,2)*A);
  ptr_map_insert<ScalarResult>(*results) ("cs", cs, "Lateral forces coefficient", "with respect to projected frontal area and forward velocity", "");

  double Pe=Rtot(Rtot.n_rows-1) * p().operation.v;
  ptr_map_insert<ScalarResult>(*results) ("Pe", Pe, "Effective power $P_e=R_{tot} v$", "", "W");

  ap.message("Creating resistance plot");
  // Resistance convergence
  addPlot
  (
    results, executionPath(), "chartResistance",
    "Iteration", "F [N]",
    {
      PlotCurve( arma::mat(join_rows(t, Rtot)),  "Rtot", "w l lw 2 t 'Total resistance'"),
      PlotCurve( arma::mat(join_rows(t, Flat)), "Flat", "w l lw 2 t 'Lateral force'"),
      PlotCurve( arma::mat(join_rows(t, L)),    "L", "w l lw 2 t 'Lifting force'")
    },
    "Convergence history of resistance force"
  );    
++ap;

  ap.message("Rendering images");
  {
    // A renderer and render window
    OpenFOAMCaseScene scene( executionPath()/"system"/"controlDict" );

    auto patches = scene.patches("object.*|floor.*");

    FieldSelection sl_field("p", FieldSupport::OnPoint, -1);
    auto sl_range=calcRange(sl_field, {patches}, {});
    auto sl_cm=createColorMap();
    FieldColor sl_fc(sl_field, sl_cm, sl_range);

    scene.addData<vtkDataSetMapper>(patches, sl_fc);
    scene.addColorBar("Pressure\n[m^2/s^2]", sl_cm);

    auto camera = scene.activeCamera();
    camera->ParallelProjectionOn();

    auto viewctr=vec3(0.5*sp().l_, 0, sp().Ldown_+0.5*sp().h_);
    camera->SetFocalPoint( toArray(viewctr) );

    {
      camera->SetViewUp( toArray(vec3(0,0,1)) );
      camera->SetPosition( toArray(viewctr+vec3(-10.0*sp().l_,0,0)) );

      auto img = executionPath() / "pressureContour_front.png";
//      scene.fitAll();
      scene.setParallelScale(std::pair<double,double>(sp().w_, sp().h_));
      scene.exportImage(img);
      results->insert(img.filename().stem().string(),
        std::unique_ptr<Image>(new Image
        (
        executionPath(), img.filename(),
        "Pressure contour (front view)", ""
      )));
    }
  ++ap;

    {
      camera->SetViewUp( toArray(vec3(0,0,1)) );
      camera->SetPosition( toArray(viewctr+vec3(0,-10.0*sp().w_,0)) );

      auto img = executionPath() / "pressureContour_side.png";
//      scene.fitAll();
      scene.setParallelScale(std::pair<double,double>(sp().l_, sp().h_));
      scene.exportImage(img);
      results->insert(img.filename().stem().string(),
        std::unique_ptr<Image>(new Image
        (
        executionPath(), img.filename(),
        "Pressure contour (side view)", ""
      )));
    }
  ++ap;

    {
      camera->SetViewUp( toArray(vec3(0,1,0)) );
      camera->SetPosition( toArray(viewctr+vec3(0,0,10.0*sp().h_)) );

      auto img = executionPath() / "pressureContour_top.png";
//      scene.fitAll();
      scene.setParallelScale(std::pair<double,double>(sp().l_, sp().w_));
      scene.exportImage(img);
      results->insert(img.filename().stem().string(),
        std::unique_ptr<Image>(new Image
        (
        executionPath(), img.filename(),
        "Pressure contour (top view)", ""
      )));
    }
  ++ap;

    {
      camera->SetViewUp( toArray(vec3(0,0,1)) );
      camera->SetPosition( toArray(viewctr+10.*vec3(-sp().l_,-sp().w_,sp().h_)) );

      auto img = executionPath() / "pressureContour_diag.png";
//      scene.fitAll();
      double f=sqrt(2.);
      scene.setParallelScale(std::pair<double,double>(
                               std::max(f*sp().l_, f*sp().w_),
                               std::max(f*sp().l_, f*sp().h_)
                               ));
      scene.exportImage(img);
      results->insert(img.filename().stem().string(),
        std::unique_ptr<Image>(new Image
        (
        executionPath(), img.filename(),
        "Pressure contour (isometric view)", ""
      )));
    }
  ++ap;

    auto im = scene.internalMesh();

    {
      auto seeds = vtkSmartPointer<vtkPointSource>::New();
      seeds->SetCenter(toArray(vec3(0.5*sp().l_, 0.5*sp().w_, sp().Ldown_+0.5*sp().h_)));
      seeds->SetRadius(0.2*sp().Lref_);
      seeds->SetDistributionToUniform();
      seeds->SetNumberOfPoints(100);

      auto st = vtkSmartPointer<vtkStreamTracer>::New();
      st->SetInputData(im);
      st->SetSourceConnection(seeds->GetOutputPort());
      st->SetIntegrationDirectionToBoth();
      st->SetMaximumPropagation(10.*sp().Ldownstream_);
      st->SetInputArrayToProcess(
            0, 0, 0,
            vtkDataObject::FIELD_ASSOCIATION_POINTS,
            "U");

      st->Update();
      scene.addData<vtkPolyDataMapper>(st->GetOutput(), vec3(0.5,0.5,0.5));
    }
  ++ap;

    {
      auto seeds = vtkSmartPointer<vtkPointSource>::New();
      seeds->SetCenter(toArray(vec3(0.5*sp().l_, -0.5*sp().w_, sp().Ldown_+0.5*sp().h_)));
      seeds->SetRadius(0.2*sp().Lref_);
      seeds->SetDistributionToUniform();
      seeds->SetNumberOfPoints(100);

      auto st = vtkSmartPointer<vtkStreamTracer>::New();
      st->SetInputData(im);
      st->SetSourceConnection(seeds->GetOutputPort());
      st->SetIntegrationDirectionToBoth();
      st->SetMaximumPropagation(10.*sp().Ldownstream_);
      st->SetInputArrayToProcess(
            0, 0, 0,
            vtkDataObject::FIELD_ASSOCIATION_POINTS,
            "U");

      st->Update();
      scene.addData<vtkPolyDataMapper>(st->GetOutput(), vec3(0.5,0.5,0.5));
    }
  ++ap;


    {
      camera->SetViewUp( toArray(vec3(0,0,1)) );
      camera->SetPosition( toArray(viewctr+vec3(-10.0*sp().l_,0,0)) );

      auto img = executionPath() / "streamLines_front.png";
//      scene.fitAll();
      scene.setParallelScale(std::pair<double,double>(2.*sp().w_, 2.*sp().h_));
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
      camera->SetPosition( toArray(viewctr+vec3(0,-10.0*sp().w_,0)) );

      auto img = executionPath() / "streamLines_side.png";
//      scene.fitAll();
      scene.setParallelScale(std::pair<double,double>(2.*sp().l_, 2.*sp().h_));
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
      camera->SetViewUp( toArray(vec3(0,1,0)) );
      camera->SetPosition( toArray(viewctr+vec3(0,0,10.0*sp().h_)) );

      auto img = executionPath() / "streamLines_top.png";
//      scene.fitAll();
      scene.setParallelScale(std::pair<double,double>(2.*sp().l_, 2.*sp().w_));
      scene.exportImage(img);
      results->insert(img.filename().stem().string(),
        std::unique_ptr<Image>(new Image
        (
        executionPath(), img.filename(),
        "Stream lines (top view)", ""
      )));
    }
  ++ap;

    {
      camera->SetViewUp( toArray(vec3(0,0,1)) );
      camera->SetPosition( toArray(viewctr+10.*vec3(-sp().l_,-sp().w_,sp().h_)) );

      auto img = executionPath() / "streamLines_diag.png";
//      scene.fitAll();
      double f=sqrt(2.);
      scene.setParallelScale(std::pair<double,double>(
                               2.*std::max(f*sp().l_, f*sp().w_),
                               2.*std::max(f*sp().l_, f*sp().h_)
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

  }


  return results;
}


    
}
