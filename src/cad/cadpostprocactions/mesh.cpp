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


AIS_InteractiveObject* Mesh::createAISRepr() const
{
  checkForBuildDuringAccess();
  return NULL;
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
  c.doMeshing(volname_, outpath_/*, true*/);
}

}
}