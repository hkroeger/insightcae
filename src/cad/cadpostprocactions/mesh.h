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

#ifndef INSIGHT_CAD_MESH_H
#define INSIGHT_CAD_MESH_H

#include "base/boost_include.h"
#include "cadtypes.h"
#include "cadparameters.h"
#include "cadpostprocaction.h"



namespace insight 
{
namespace cad 
{

class GmshCase;

typedef boost::fusion::vector3<std::string, FeatureSetPtr, boost::optional<ScalarPtr> > GroupDesc;
typedef std::vector<GroupDesc> GroupsDesc;

typedef boost::fusion::vector2<std::string, VectorPtr> NamedVertex;
typedef std::vector<NamedVertex> NamedVertices;

typedef boost::fusion::vector<
    insight::cad::GroupsDesc, //vertexGroups,
    insight::cad::GroupsDesc, //edgeGroups,
    insight::cad::GroupsDesc, //faceGroups,
    insight::cad::GroupsDesc //solidGroups,
  > GroupDefinitions;

typedef boost::fusion::vector3<
    VectorPtr /*location*/, ScalarPtr /*D*/, ScalarPtr /*Linside*/
    > MeshSizeBall;

class Mesh 
: public insight::cad::PostprocAction
{
protected:
  boost::filesystem::path outpath_;
  FeaturePtr model_;
  std::string volname_;
  ScalarPtr Lmax_, Lmin_;
  bool quad_;
  GroupsDesc vertexGroups_;
  GroupsDesc edgeGroups_;
  GroupsDesc faceGroups_;
  GroupsDesc solidGroups_;
  NamedVertices namedVertices_;
  std::vector<MeshSizeBall> meshSizeBalls_;
  bool keepTmpDir_;
  
  size_t calcHash() const override;

  virtual void setupGmshCase(GmshCase& c);
  void build() override;

public:
  declareType("Mesh");
  Mesh
  (
    const boost::filesystem::path& outpath,
    FeaturePtr model,
    boost::fusion::vector<ScalarPtr,ScalarPtr> L,
    bool quad,
    const GroupDefinitions& v_e_f_s_groups,
    const NamedVertices& namedVertices,
    const std::vector<MeshSizeBall>& meshSizeBalls,
    bool keepTmpDir=false
  );
  

//  Handle_AIS_InteractiveObject createAISRepr() const override;
  void write(std::ostream& ) const override;
};




typedef boost::fusion::vector<
    insight::cad::GroupsDesc, //vertexGroups,
    insight::cad::GroupsDesc, //edgeGroups,
    insight::cad::GroupsDesc, //baseFaceGroups,
    insight::cad::GroupsDesc, //topFaceGroups,
    insight::cad::GroupsDesc //solidGroups,
  > ExtrudedGroupDefinitions;




class ExtrudedMesh
: public Mesh
{

protected:
  ScalarPtr h_, nLayers_;
  std::vector<std::pair<std::string, cad::FeatureSetPtr> > namedBottomFaces_, namedTopFaces_, namedLateralEdges_;

  void build() override;

public:
  declareType("ExtrudedMesh");
  ExtrudedMesh
  (
    const boost::filesystem::path& outpath,
    FeaturePtr model,
    boost::fusion::vector<ScalarPtr,ScalarPtr,ScalarPtr,ScalarPtr> L_h_nLayers,
    bool quad,
    const ExtrudedGroupDefinitions& v_e_bf_tf_s_groups,
    const NamedVertices& namedVertices,
    bool keepTmpDir=false
  );
};





typedef boost::fusion::vector5<FeaturePtr, std::string, ScalarPtr, boost::optional<boost::fusion::vector2<ScalarPtr, ScalarPtr> >, boost::optional<ScalarPtr> > GeometryDesc;
typedef std::vector<GeometryDesc> GeometrysDesc;

typedef boost::fusion::vector3<std::string, FeatureSetPtr, ScalarPtr> EdgeRefineDesc;
typedef std::vector<EdgeRefineDesc> EdgeRefineDescs;




class SnappyHexMesh 
: public insight::cad::PostprocAction
{
  boost::filesystem::path outpath_;
  std::string OFEname_;
  VectorPtr PiM_;
  ScalarPtr templCellSize_;
  GeometrysDesc geometries_;
  EdgeRefineDescs edgerefines_;
  
  virtual size_t calcHash() const;
  virtual void build();

public:
  declareType("SnappyHexMesh");

  SnappyHexMesh
  (
    const boost::filesystem::path& outpath,
    const std::string OFEname,
    VectorPtr PiM,
    ScalarPtr templCellSize,
    GeometrysDesc geometries,
    boost::optional<EdgeRefineDescs> edgerefines
  );
  

//  virtual Handle_AIS_InteractiveObject createAISRepr() const;
  virtual void write(std::ostream& ) const;
};




}
}

#endif // INSIGHT_CAD_MESH_H
