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

#include "boost/ptr_container/ptr_map.hpp"
#include "solidmodel.h"

namespace insight {
namespace cad {
  
class GmshCase
{
public:
  typedef std::map<std::string, FeatureSetPtr> NamedFeatureSet;
  
private:
  const SolidModel& part_;
  double Lmax_, Lmin_;
  
  NamedFeatureSet namedVertices_, namedEdges_, namedFaces_;
  std::vector<std::string> options_;
  
  int elementOrder_;
  int secondOrderLinear_;
  
  int additionalPoints_;
  
public:
  GmshCase(const SolidModel& part, double Lmax=500., double Lmin=0.1);
  
  void nameVertices(const std::string& name, const FeatureSet& vertices);
  void nameEdges(const std::string& name, const FeatureSet& edges);
  void nameFaces(const std::string& name, const FeatureSet& faces);

  void addSingleNamedVertex(const std::string& vn, const arma::mat& p);
  void setVertexLen(const std::string& vn, double L);
  void setEdgeLen(const std::string& en, double L);
  void setFaceEdgeLen(const std::string& fn, double L);
  
  void doMeshing
  (
    const std::string& volname,
    const boost::filesystem::path& outputMeshFile,
    bool keeptmpdir=false
  );
};
  
}
}

#endif