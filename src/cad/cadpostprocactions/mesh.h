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

typedef boost::fusion::vector3<std::string, FeatureSetPtr, boost::optional<ScalarPtr> > GroupDesc;
typedef std::vector<GroupDesc> GroupsDesc;

typedef boost::fusion::vector2<std::string, VectorPtr> NamedVertex;
typedef std::vector<NamedVertex> NamedVertices;

class Mesh 
: public insight::cad::PostprocAction
{
  boost::filesystem::path outpath_;
  FeaturePtr model_;
  std::string volname_;
  std::vector<ScalarPtr> L_;
  bool quad_;
  GroupsDesc vertexGroups_;
  GroupsDesc edgeGroups_;
  GroupsDesc faceGroups_;
  NamedVertices namedVertices_;
  
public:
  Mesh
  (
    const boost::filesystem::path& outpath,
    FeaturePtr model,
    const std::string& volname,
    std::vector<ScalarPtr> L,
    bool quad,
    const GroupsDesc& vertexGroups,
    const GroupsDesc& edgeGroups,
    const GroupsDesc& faceGroups,
    const NamedVertices& namedVertices
  );
  
  virtual void build();

  virtual Handle_AIS_InteractiveObject createAISRepr(const Handle_AIS_InteractiveContext& context) const;
  virtual void write(std::ostream& ) const;
};


}
}

#endif // INSIGHT_CAD_MESH_H
