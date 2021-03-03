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

#ifndef INSIGHT_CAD_MESHING_H
#define INSIGHT_CAD_MESHING_H

#ifndef Q_MOC_RUN
#include "boost/ptr_container/ptr_map.hpp"
#endif
#include "cadfeature.h"
#include "base/tools.h"

#include <list>

namespace insight {
namespace cad {

class GmshCase;

std::ostream& operator<<(std::ostream& os, GmshCase& gc);

class GmshCase
    : std::list<std::string>
{
  friend std::ostream& operator<<(std::ostream& os, GmshCase& gc);

public:
  typedef std::map<std::string, FeatureSetPtr> NamedFeatureSet;
  
private:
  CaseDirectory workDir_;

protected:
  iterator
    endOfPreamble_,
    endOfExternalGeometryMerging_,
      endOfNamedVerticesDefinition_,
      endOfNamedEdgesDefinition_,
      endOfNamedFacesDefinition_,
      endOfNamedSolidsDefinition_,
    endOfGeometryDefinition_,
    endOfMeshingOptions_,
    endOfMeshingActions_;

  ConstFeaturePtr part_;
  
  int additionalPoints_;

  std::string executableName_ = "gmsh";

  boost::filesystem::path outputMeshFile_;
  
public:
  GmshCase(
      ConstFeaturePtr part,
      const boost::filesystem::path& outputMeshFile,
      double Lmax=500., double Lmin=0.1,
      const std::string& exeName="gmsh",
      bool keepDir=false
      );

  void insertLinesBefore(
      std::list<std::string>::iterator i,
      const std::vector<std::string>& lines
      );

  std::set<int> findNamedDefinition(const std::string& keyword, const std::string& name) const;
  std::set<int> findNamedEdges(const std::string& name) const;
  std::set<int> findNamedFaces(const std::string& name) const;
  std::set<int> findNamedSolids(const std::string& name) const;

  void setLinear();
  void setQuadratic();
  void setMinimumCirclePoints(int mp);
  
  void nameVertices(const std::string& name, const FeatureSet& vertices);
  void nameEdges(const std::string& name, const FeatureSet& edges);
  void nameFaces(const std::string& name, const FeatureSet& faces);
  void nameSolids(const std::string& name, const FeatureSet& solids);

  void addSingleNamedVertex(const std::string& vn, const arma::mat& p);
  void setVertexLen(const std::string& vn, double L);
  void setEdgeLen(const std::string& en, double L);
  void setFaceEdgeLen(const std::string& fn, double L);
  
  void doMeshing();
};
  


class SurfaceGmshCase
    : public cad::GmshCase
{
public:
  SurfaceGmshCase(
      cad::ConstFeaturePtr part,
      const boost::filesystem::path& outputMeshFile,
      double Lmax, double Lmin,
      const std::string& name,
      bool keepDir=false
      );
};




class SheetExtrusionGmshCase
    : public cad::GmshCase
{

public:
  typedef std::pair<std::string, cad::FeatureSetPtr> NamedEntity;

protected:
  std::map<cad::FeatureID, std::string> namedBottomFaces_, namedTopFaces_, namedLateralEdges_;

public:
  SheetExtrusionGmshCase(
      cad::ConstFeaturePtr part,
      const std::string& solidName,
      const boost::filesystem::path& outputMeshFile,
      double L, double h, int nLayers,
      const std::vector<NamedEntity>& namedBottomFaces,
      const std::vector<NamedEntity>& namedTopFaces,
      const std::vector<NamedEntity>& namedLateralEdges,
      bool keepDir=false
      );
};


}
}

#endif
