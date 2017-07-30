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

#include "meshing.h"

#include "base/boost_include.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/snappyhexmesh.h"

#include "openfoam/blockmesh_templates.h"
#include "openfoam/numericscaseelements.h"

namespace insight
{
namespace cad
{
  
Mesh::Mesh
(
  const boost::filesystem::path& outpath, 
  insight::cad::FeaturePtr model, 
  const std::string& volname, 
  std::vector< insight::cad::ScalarPtr > L,
  bool quad, 
  const insight::cad::GroupsDesc& vertexGroups, 
  const insight::cad::GroupsDesc& edgeGroups, 
  const insight::cad::GroupsDesc& faceGroups, 
  const insight::cad::NamedVertices& namedVertices
)
: model_(model),
  outpath_(outpath),
  volname_(volname),
  L_(L),
  quad_(quad),
  vertexGroups_(vertexGroups),
  edgeGroups_(edgeGroups),
  faceGroups_(faceGroups),
  namedVertices_(namedVertices)
{}


Handle_AIS_InteractiveObject Mesh::createAISRepr(const Handle_AIS_InteractiveContext&) const
{
  checkForBuildDuringAccess();
  return Handle_AIS_InteractiveObject();
}

void Mesh::write(std::ostream& ) const
{
}

void Mesh::build()
{
  GmshCase c(*model_, L_[0]->value(), L_[1]->value());
  if (!quad_) c.setLinear();
  BOOST_FOREACH(const GroupDesc& gd, vertexGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameVertices(gname, *gfs);
  }
  BOOST_FOREACH(const GroupDesc& gd, edgeGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameEdges(gname, *gfs);
  }
  BOOST_FOREACH(const GroupDesc& gd, faceGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const FeatureSetPtr& gfs=boost::fusion::at_c<1>(gd);
    c.nameFaces(gname, *gfs);
  }
  BOOST_FOREACH(const NamedVertex& gd, namedVertices_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    const arma::mat& loc=boost::fusion::at_c<1>(gd)->value();
    c.addSingleNamedVertex(gname, loc);
  }
  
  BOOST_FOREACH(const GroupDesc& gd, vertexGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<ScalarPtr> gs=boost::fusion::at_c<2>(gd))
    {
      cout<<"set vertex "<<gname<<" to L="<<(*gs)->value()<<endl;
      c.setVertexLen(gname, (*gs)->value());
    }
  }
  BOOST_FOREACH(const GroupDesc& gd, edgeGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<ScalarPtr> gs=boost::fusion::at_c<2>(gd))
    {
      c.setEdgeLen(gname, (*gs)->value());
    }
  }
  BOOST_FOREACH(const GroupDesc& gd, faceGroups_)
  {
    const std::string& gname=boost::fusion::at_c<0>(gd);
    if (boost::optional<ScalarPtr> gs=boost::fusion::at_c<2>(gd))
    {
      c.setFaceEdgeLen(gname, (*gs)->value());
    }
  }
  c.doMeshing(volname_, outpath_, true);
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
    
    BOOST_FOREACH(const GeometryDesc& geom, geometries_)
    {
        const FeaturePtr geo = boost::fusion::at_c<0>(geom);
        const std::string& name = boost::fusion::at_c<1>(geom);
        
        boost::filesystem::path filepath = outpath_/(name+".stlb");
        geo->saveAs(filepath);
        
        arma::mat bb = geo->modelBndBox();
        for (int i=0; i<3; i++)
        {
            pmin(i) = std::min( pmin(i), bb(i,0) );
            pmax(i) = std::max( pmax(i), bb(i,1) );
        }
        
        int minlevel=0, maxlevel=0;
        if (boost::fusion::at_c<2>(geom))
        {
            const boost::fusion::vector2<ScalarPtr, ScalarPtr>& levels= *boost::fusion::at_c<2>(geom);
            minlevel=boost::fusion::at_c<0>(levels)->value();
            maxlevel=boost::fusion::at_c<1>(levels)->value();
        }
        
        int nlayers=0;
        if (boost::fusion::at_c<3>(geom))
        {
            nlayers=(*boost::fusion::at_c<3>(geom))->value();
        }
        
        shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(  
            new snappyHexMeshFeats::Geometry(snappyHexMeshFeats::Geometry::Parameters()
            .set_name(name)
            .set_minLevel(minlevel)
            .set_maxLevel(maxlevel)
            .set_nLayers(nlayers)
            //.set_regionRefinements(rl)
            
            .set_fileName(filepath)
        )));
    }
    
    BOOST_FOREACH(const EdgeRefineDesc& edgref, edgerefines_)
    {
        const std::string& name = boost::fusion::at_c<0>(edgref);
        FeatureSetPtr fsp = boost::fusion::at_c<1>(edgref);
        int level = boost::fusion::at_c<2>(edgref)->value();
        
        boost::filesystem::path filepath = outpath_/(name+".eMesh");
        fsp->model()->exportEMesh(filepath, *fsp);

        shm_cfg.features.push_back(snappyHexMeshFeats::FeaturePtr(
            new snappyHexMeshFeats::ExplicitFeatureCurve(snappyHexMeshFeats::ExplicitFeatureCurve::Parameters()
            .set_level(level)
            .set_fileName(filepath)
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
    bmd_p.mesh.nx=nx;
    bmd_p.mesh.ny=ny;
    bmd_p.mesh.nz=nz;
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

Handle_AIS_InteractiveObject SnappyHexMesh::createAISRepr(const Handle_AIS_InteractiveContext&) const
{
  checkForBuildDuringAccess();
  return Handle_AIS_InteractiveObject();
}

void SnappyHexMesh::write(std::ostream& ) const
{
}
  
}
}
