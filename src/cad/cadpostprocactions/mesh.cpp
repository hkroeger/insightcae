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
 */

#include "mesh.h"

#include "cadfeature.h"
#include "meshing.h"

#include "base/boost_include.h"

#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/snappyhexmesh.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"

#include "openfoam/blockmesh_templates.h"

using namespace std;

namespace insight
{
namespace cad
{

size_t Mesh::calcHash() const
{
  ParameterListHash h;
#warning compute hash
  return h.getHash();
}

defineType(Mesh);
  
Mesh::Mesh
(
  const boost::filesystem::path& outpath, 
  insight::cad::FeaturePtr model,
  boost::fusion::vector< ScalarPtr,ScalarPtr > L,
  bool quad, 
  const GroupDefinitions& v_e_f_s_groups,
  const insight::cad::NamedVertices& namedVertices,
  const std::vector<MeshSizeBall>& meshSizeBalls,
  bool keepTmpDir
)
: 
  outpath_(outpath),
  model_(model),
//   volname_(volname),
  Lmax_(boost::fusion::at_c<0>(L)), Lmin_(boost::fusion::at_c<1>(L)),
  quad_(quad),
  vertexGroups_(boost::fusion::at_c<0>(v_e_f_s_groups)),
  edgeGroups_(boost::fusion::at_c<1>(v_e_f_s_groups)),
  faceGroups_(boost::fusion::at_c<2>(v_e_f_s_groups)),
  solidGroups_(boost::fusion::at_c<3>(v_e_f_s_groups)),
  namedVertices_(namedVertices),
  meshSizeBalls_(meshSizeBalls),
  keepTmpDir_(keepTmpDir)
{}


//Handle_AIS_InteractiveObject Mesh::createAISRepr() const
//{
//  checkForBuildDuringAccess();
//  return Handle_AIS_InteractiveObject();
//}

void Mesh::write(std::ostream& ) const
{}

void Mesh::setupGmshCase(GmshCase& c)
{
  if (quad_)
    c.setQuadratic();
  else
    c.setLinear();

  for (const GroupDesc& gd: vertexGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameVertices(gname, *gfs);
  }
  for (const GroupDesc& gd: edgeGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameEdges(gname, *gfs);
  }
  for (const GroupDesc& gd: faceGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameFaces(gname, *gfs);
  }
  for (const GroupDesc& gd: solidGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameSolids(gname, *gfs);
  }
  for (const NamedVertex& gd: namedVertices_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const arma::mat& loc=boost::fusion::at_c<1>(gd)->value();
    c.addSingleNamedVertex(gname, loc);
  }

  for (const GroupDesc& gd: vertexGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<ScalarPtr> gs=boost::fusion::at_c<2>(gd))
    {
      cout<<"set vertex "<<gname<<" to L="<<(*gs)->value()<<endl;
      c.setVertexLen(gname, (*gs)->value());
    }
  }
  for (const GroupDesc& gd: edgeGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<ScalarPtr> gs=boost::fusion::at_c<2>(gd))
    {
      c.setEdgeLen(gname, (*gs)->value());
    }
  }
  for (const GroupDesc& gd: faceGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<ScalarPtr> gs=boost::fusion::at_c<2>(gd))
    {
      c.setFaceEdgeLen(gname, (*gs)->value());
    }
  }
}

void Mesh::build()
{
  GmshCase c(model_, outpath_, Lmax_->value(), Lmin_->value(), keepTmpDir_);
  setupGmshCase(c);

  std::vector<std::string> addcode;
  std::vector<std::string> fis;
  for (auto func: boost::adaptors::index(meshSizeBalls_))
  {
      auto location = boost::fusion::get<0>(func.value())->value();
      auto D = boost::fusion::get<1>(func.value())->value();
      auto Linside = boost::fusion::get<2>(func.value())->value();
      fis.push_back( boost::lexical_cast<std::string>(func.index()+1) );
      auto fi = "Field["+fis.back()+"]";
      addcode.insert(
          addcode.end(),
          {
           fi+"=Ball",
           str(boost::format(fi+".Radius=%g")%(0.5*D)),
           str(boost::format(fi+".VIn=%g")%Linside),
           str(boost::format(fi+".VOut=%g")%1e22),
           str(boost::format(fi+".XCenter=%g")%location(0)),
           str(boost::format(fi+".YCenter=%g")%location(1)),
           str(boost::format(fi+".ZCenter=%g")%location(2))
          });
  }
  {
      auto fii=boost::lexical_cast<std::string>(meshSizeBalls_.size()+1);
      auto fi = "Field["+fii+"]";
      addcode.insert(
          addcode.end(),
          {
              fi+"=Min",
              fi+".FieldsList={"+boost::join(fis, ", ")+"}",
              "Background Field = "+fii
          });
  }
  c.insertLinesBefore(
      c.endOfMeshingOptions_, addcode);

  c.doMeshing(std::thread::hardware_concurrency());
}





defineType(ExtrudedMesh);

ExtrudedMesh::ExtrudedMesh
(
  const boost::filesystem::path& outpath,
  insight::cad::FeaturePtr model,
  boost::fusion::vector< ScalarPtr,ScalarPtr,ScalarPtr,ScalarPtr > L_h_nLayers,
  bool quad,
  const ExtrudedGroupDefinitions& v_e_bf_tf_s_groups,
  const insight::cad::NamedVertices& namedVertices,
  bool keepTmpDir
)
: Mesh(
    outpath,
    model,
    boost::fusion::vector<ScalarPtr,ScalarPtr>(
      boost::fusion::at_c<0>(L_h_nLayers),
      boost::fusion::at_c<1>(L_h_nLayers)
      ),
    quad,
    GroupDefinitions(
      boost::fusion::at_c<0>(v_e_bf_tf_s_groups),
      boost::fusion::at_c<1>(v_e_bf_tf_s_groups),
      boost::fusion::at_c<2>(v_e_bf_tf_s_groups),
      boost::fusion::at_c<4>(v_e_bf_tf_s_groups)
      ),
    namedVertices,
    {},
    keepTmpDir
    ),
  h_(boost::fusion::at_c<2>(L_h_nLayers)),
  nLayers_(boost::fusion::at_c<3>(L_h_nLayers))
{
  auto transf = [&](const GroupsDesc& g, std::vector<SheetExtrusionGmshCase::NamedEntity>& tg)
  {
    std::transform(
          g.begin(), g.end(),
          std::back_inserter(tg),
          [&](const GroupDesc& a) -> SheetExtrusionGmshCase::NamedEntity
          {
            return SheetExtrusionGmshCase::NamedEntity
            (
              boost::fusion::at_c<0>(a),
              boost::fusion::at_c<1>(a)
            );
          }
    );
  };

  transf( boost::fusion::at_c<2>(v_e_bf_tf_s_groups), namedBottomFaces_ );
  transf( boost::fusion::at_c<3>(v_e_bf_tf_s_groups), namedTopFaces_ );
  transf( boost::fusion::at_c<1>(v_e_bf_tf_s_groups), namedLateralEdges_ );
}




void ExtrudedMesh::build()
{

  SheetExtrusionGmshCase c(
        model_, "G_3D_1", outpath_,
        Lmin_->value(),
        h_->value(),
        nLayers_->value(),
        namedBottomFaces_, namedTopFaces_, namedLateralEdges_,
        keepTmpDir_
      );
  setupGmshCase(c);
  c.doMeshing();
}



defineType(SnappyHexMesh);

size_t SnappyHexMesh::calcHash() const
{
  ParameterListHash h;
#warning compute hash!
  return h.getHash();
}


SnappyHexMesh::SnappyHexMesh
(
    const boost::filesystem::path& outpath,
    const std::string OFEname,
    VectorPtr PiM,
    ScalarPtr templCellSize,
    GeometrysDesc geometries,
    boost::optional<EdgeRefineDescs> edgerefines
)
: outpath_(outpath),
  OFEname_(OFEname),
  PiM_(PiM),
  templCellSize_(templCellSize),
  geometries_(geometries)
{
    if (edgerefines) edgerefines_=*edgerefines;
}

void SnappyHexMesh::build()
{
    boost::filesystem::create_directory(outpath_); // create dir, if not existing
        
    OpenFOAMCase ofc( OFEs::get(OFEname_) );

    snappyHexMeshConfiguration::Parameters shm_cfg;

    arma::mat pmin = vec3(1e10, 1e10, 1e10);
    arma::mat pmax = vec3(-1e10, -1e10, -1e10);
    
    for (const GeometryDesc& geom: geometries_)
    {
        const FeaturePtr geo = boost::fusion::at_c<0>(geom);
        const std::string& name = boost::fusion::at_c<1>(geom);
        
        boost::filesystem::path filepath = outpath_/(name+".stlb");
        ScalarPtr res= boost::fusion::at_c<2>(geom);
        if (res)
        {
            geo->exportSTL(filepath, res->value());
        }
        else
        {
            geo->saveAs(filepath);
        }
        
        arma::mat bb = geo->modelBndBox();
        for (int i=0; i<3; i++)
        {
            pmin(i) = std::min( pmin(i), bb(i,0) );
            pmax(i) = std::max( pmax(i), bb(i,1) );
        }
        
        int minlevel=0, maxlevel=0;
        if (boost::fusion::at_c<3>(geom))
        {
            const boost::fusion::vector2<ScalarPtr, ScalarPtr>& levels= *boost::fusion::at_c<3>(geom);
            minlevel=boost::fusion::at_c<0>(levels)->value();
            maxlevel=boost::fusion::at_c<1>(levels)->value();
        }
        
        int nlayers=0;
        if (boost::fusion::at_c<4>(geom))
        {
            nlayers=(*boost::fusion::at_c<4>(geom))->value();
        }
        
        shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(  
            new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
            .set_minLevel(minlevel)
            .set_maxLevel(maxlevel)
            .set_nLayers(nlayers)
            //.set_regionRefinements(rl)

            .set_fileName(make_filepath(filepath))
            .set_name(name)
        )));
    }
    
    for (const EdgeRefineDesc& edgref: edgerefines_)
    {
        const std::string& name = boost::fusion::at_c<0>(edgref);
        FeatureSetPtr fsp = boost::fusion::at_c<1>(edgref);
        int level = boost::fusion::at_c<2>(edgref)->value();
        
        boost::filesystem::path filepath = outpath_/(name+".eMesh");
        fsp->model()->exportEMesh(filepath, *fsp);

        shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
            new snappyHexMeshFeats::ExplicitFeatureCurve(snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
            .set_level(level)
            .set_fileName(make_filepath(filepath))
        )));
    }
    
    shm_cfg
      .set_relativeSizes(true)
      .set_stopOnBadPrismLayer(false)
      .set_erlayer(1.3)
      .set_tlayer(0.5);
  
    shm_cfg.PiM.push_back(PiM_->value());
    
    ofc.insert(new MeshingNumerics(ofc, MeshingNumerics::Parameters()
//                                   .set_np(np)
                                 ));
    
    {
     double L = pmax(0)-pmin(0), W=pmax(1)-pmin(1), H=pmax(2)-pmin(2);
     double maxs = std::max(L, std::max(W, H));
     double add = 0.05*maxs;
     pmin -= add;
     pmax += add;
    }
    
    double L = pmax(0)-pmin(0), W=pmax(1)-pmin(1), H=pmax(2)-pmin(2);
    int nx=std::max(1, int(ceil(L/templCellSize_->value())) );
    int ny=std::max(1, int(ceil(W/templCellSize_->value())) );
    int nz=std::max(1, int(ceil(H/templCellSize_->value())) );
    
    bmd::blockMeshDict_Box::Parameters bmd_p;
    bmd_p.geometry.p0=pmin;
    bmd_p.geometry.L=L;
    bmd_p.geometry.W=W;
    bmd_p.geometry.H=H;
    {
      bmd::blockMeshDict_Box::Parameters::mesh_type::resolution_individual_type res;
      res.nx=nx;
      res.ny=ny;
      res.nz=nz;
      bmd_p.mesh.resolution=res;
    }
    ofc.insert(new bmd::blockMeshDict_Box(ofc, bmd_p));
    
    ofc.createOnDisk(outpath_);
    ofc.executeCommand(outpath_, "blockMesh");
    
    snappyHexMesh
    (
        ofc, outpath_,
        shm_cfg,
        true,
        false,
        false
    );
}

//Handle_AIS_InteractiveObject SnappyHexMesh::createAISRepr() const
//{
//  checkForBuildDuringAccess();
//  return Handle_AIS_InteractiveObject();
//}

void SnappyHexMesh::write(std::ostream& ) const
{
}
  
}
}
