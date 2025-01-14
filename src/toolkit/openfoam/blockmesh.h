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


#ifndef INSIGHT_BLOCKMESH_H
#define INSIGHT_BLOCKMESH_H

#include "openfoam/caseelements/openfoamcaseelement.h"

#include "base/linearalgebra.h"

#include "boost/variant.hpp"

#include <cmath>
#include <vector>
#include <map>
#include <armadillo>

#include "openfoam/blockmesh/point.h"
#include "openfoam/blockmesh/gradinganalyzer.h"

#include "openfoam/blockmesh/discretecurve.h"
#include "openfoam/blockmesh/edge.h"
#include "openfoam/blockmesh/patch.h"
#include "openfoam/blockmesh/patch2d.h"
#include "openfoam/blockmesh/block.h"
#include "openfoam/blockmesh/block2d.h"

#include "openfoam/blockmesh/arcedge.h"
#include "openfoam/blockmesh/circularedge_center.h"
#include "openfoam/blockmesh/circularedge.h"
#include "openfoam/blockmesh/ellipseedge.h"
#include "openfoam/blockmesh/splineedge.h"

#include "openfoam/blockmesh/transform2d.h"
#include "openfoam/blockmesh/plane2d.h"
#include "openfoam/blockmesh/wedge2d.h"

#include "openfoam/blockmesh/geometry.h"
#include "openfoam/blockmesh/projectededge.h"
#include "openfoam/blockmesh/projectedface.h"


namespace insight {
namespace bmd {



class blockMesh 
: public OpenFOAMCaseElement
{

public:
  typedef boost::ptr_map<std::string, Patch> PatchMap;

protected:
  double scaleFactor_;
  std::string defaultPatchName_;
  std::string defaultPatchType_;
  
  PointMap allPoints_;
  boost::ptr_vector<Block> allBlocks_;
  boost::ptr_vector<Edge> allEdges_;
  PatchMap allPatches_;
  std::vector<Geometry> geometries_;
  std::map<Point,std::string> projectedVertices_;
  std::vector<ProjectedFace> projectedFaces_;
  
public:
  blockMesh(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

  void copy(const blockMesh& other);
  
  void setScaleFactor(double sf);
  void setDefaultPatch(const std::string& name, std::string type="patch");

  void addGeometry(const Geometry& geo);
  const std::vector<Geometry>& allGeometry() const;

  void addProjectedVertex(const Point& pf, const std::string& geometryLabel);

  void addProjectedFace(const ProjectedFace& pf);
  const std::vector<ProjectedFace>& allProjectedFaces() const;

  inline const boost::ptr_vector<Block>& allBlocks() const { return allBlocks_; }
  inline const boost::ptr_vector<Edge>& allEdges() const { return allEdges_; }
  inline const PatchMap& allPatches() const { return allPatches_; }
  
  inline Patch& patch(const std::string& name) { return allPatches_.at(name); }
  
  inline void addPoint(const Point& p) 
  { 
    allPoints_[p]=0; 
  }
  
  void numberVertices(PointMap& pts) const;
  
  inline Block& addBlock(Block *block) 
  { 
    block->registerPoints(*this);
    allBlocks_.push_back(block);
    return *block;
  }
  
  Edge& addEdge(Edge *edge);
  
  inline Patch& addPatch(const std::string& name, Patch *patch) 
  { 
    if (name=="")
      throw insight::Exception("Empty patch names are not allowed!");
    
    std::string key(name);
    allPatches_.insert(key, patch); 
    return *patch; 
  }

  template<class EdgeType = Edge>
  const EdgeType* edgeBetween(const Point& p1, const Point& p2) const
  {
      for (const auto& e: allEdges_)
      {
          if (e.connectsPoints(p1, p2))
          {
              auto *te = dynamic_cast<const EdgeType*>(&e);
              insight::assertion(
                          te!=nullptr,
                          "edge not of assumed type!" );
              return te;
          }
      }
      return nullptr;
  }

  bool hasEdgeBetween(const Point& p1, const Point& p2) const;
  
  /**
   * Add the given patch, if none with the same name is present.
   * If it is present, the supplied object is deleted and the existing patch is returned.
   */
  inline Patch& addOrDestroyPatch(const std::string& name, Patch *patch) 
  { 
    if (name=="")
      throw insight::Exception("Empty patch names are not allowed!");
    
    std::string key(name);
    if ( allPatches_.find(name)!=allPatches_.end())
    {
      delete patch;
      return *allPatches_.find(name)->second;
    }
    else
    {
      allPatches_.insert(key, patch); 
      return *patch; 
    }
  }
  
  void removePatch(const std::string& name);
  
  OFDictData::dict& getBlockMeshDict(insight::OFdicts& dictionaries) const;
  void addIntoDictionaries(insight::OFdicts& dictionaries) const override;

  void writeVTK(const boost::filesystem::path& fn) const;

  int nBlocks() const;
  
  static std::string category() { return "Meshing"; }
};




typedef std::shared_ptr<blockMesh> blockMeshPtr;




}

}

#endif // INSIGHT_BLOCKMESH_H
