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

#include "openfoam/blockmesh.h"
#include "base/vtktools.h"
#include "openfoam/ofdicts.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

using namespace std;
using namespace boost::assign;
using namespace arma;


namespace insight {
namespace bmd {



blockMesh::blockMesh(OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, /*"blockMesh",*/ ip.forward<Parameters>()),
  scaleFactor_(1.0),
  defaultPatchName_("defaultFaces"),
  defaultPatchType_("wall"),
  allPoints_()
{
}

void blockMesh::copy(const blockMesh& other)
{
  for (const auto& b: other.allBlocks_)
    {
      this->addBlock( b.clone() );
    }
  for (const auto& e: other.allEdges_)
    {
      if (!hasEdgeBetween(e.c0(), e.c1()))
        this->addEdge(e.clone());
    }
  for (const auto p: other.allPatches_)
    {
      auto pp=allPatches_.find(p.first);
      if (pp!=allPatches_.end())
        {
          pp->second->appendPatch(*p.second);
        }
      else
        {
          this->addPatch(p.first, p.second->clone());
        }
    }
}

void blockMesh::setScaleFactor(double sf)
{
  scaleFactor_=sf;
}

void blockMesh::setDefaultPatch(const std::string& name, std::string type)
{
  defaultPatchName_=name;
  defaultPatchType_=type;
}

void blockMesh::addGeometry(const Geometry& geo)
{
    geometries_.push_back(geo);
}

const std::vector<Geometry>& blockMesh::allGeometry() const
{
    return geometries_;
}

void blockMesh::addProjectedVertex(const Point& pf, const std::string& geometryLabel)
{
    projectedVertices_[pf]=geometryLabel;
}

void blockMesh::addProjectedFace(const ProjectedFace& pf)
{
    projectedFaces_.push_back(pf);
}

const std::vector<ProjectedFace>& blockMesh::allProjectedFaces() const
{
    return projectedFaces_;
}

void blockMesh::removePatch(const std::string& name)
{
  std::string key(name);
  auto elem = allPatches_.find(name);
  if ( elem != allPatches_.end())
  {
    allPatches_.erase(elem);
  }
}

void blockMesh::numberVertices(PointMap& pts) const
{
  int idx=0;
  for (PointMap::iterator i=pts.begin();
       i!=pts.end(); i++)
       {
	 i->second = idx++;
  }
}

Edge &blockMesh::addEdge(Edge *edge)
{
  // check if edge was already added
  for (const auto& e: allEdges_)
  {
    if (e.connectsPoints(edge->c0(), edge->c1()))
    {
      auto c0=edge->c0();
      auto c1=edge->c1();
      throw insight::Exception(
            boost::str(boost::format("There was already another edge added, which connect points [%g %g %g] and [%g %g %g]!")
                %c0(0)%c0(1)%c0(2)
                %c1(0)%c1(1)%c1(2))
            );
    }
  }

  edge->registerPoints(*this);
  allEdges_.push_back(edge);
  return *edge;
}



bool blockMesh::hasEdgeBetween(const Point& p1, const Point& p2) const
{
  return edgeBetween(p1, p2)!=nullptr;
}

OFDictData::dict& blockMesh::getBlockMeshDict(insight::OFdicts& dictionaries) const
{
  std::string bmdLoc="constant/polyMesh/blockMeshDict";
  if (OFversion()>=500)
  {
      bmdLoc="system/blockMeshDict";
  }
  return dictionaries.lookupDict(bmdLoc);
}

void blockMesh::addIntoDictionaries(insight::OFdicts& dictionaries) const
{
    PointMap pts(allPoints_);
    numberVertices(pts);

    OFDictData::dict& blockMeshDict=getBlockMeshDict(dictionaries);

    blockMeshDict["convertToMeters"]=scaleFactor_;

    OFDictData::dict def;
    def["name"]=OFDictData::data(defaultPatchName_);
    def["type"]=OFDictData::data(defaultPatchType_);
    blockMeshDict["defaultPatch"]=def;

    OFDictData::list vl;
    for (PointMap::const_iterator i=allPoints_.begin();
         i!=allPoints_.end(); i++)
    {
        auto p = i->first;
        OFDictData::list cl;
        cl += p[0], p[1], p[2];

        auto pv = projectedVertices_.find(p);
        if (pv==projectedVertices_.end())
        {
            vl.push_back( cl );
        }
        else
        {
            vl.insert(vl.end(), {
                          "project", cl, "("+pv->second+")"
                      });
        }
    }
    blockMeshDict["vertices"]=vl;

    int n_cells=0;
    OFDictData::list bl;
    std::set<Point> blockCorners;
    for (const auto& b : allBlocks_)
    {
        n_cells+=b.nCells();
        std::vector<OFDictData::data> l = b.bmdEntry(pts, OFversion());
        bl.insert( bl.end(), l.begin(), l.end() );

        auto c=b.corners();
        std::copy(c.begin(), c.end(), std::inserter(blockCorners, blockCorners.begin()));
    }
    blockMeshDict["blocks"]=bl;
    cout<<"blockMeshDict will create "<<n_cells<<" cells."<<endl;

    OFDictData::list el;
    for (const auto& e: allEdges_)
    {
        if ( blockCorners.find(e.c0())!=blockCorners.end()
             &&
             blockCorners.find(e.c1())!=blockCorners.end() )
        {
            std::vector<OFDictData::data> l = e.bmdEntry(pts, OFversion());
            el.insert( el.end(), l.begin(), l.end() );
        }
    }
    blockMeshDict["edges"]=el;

    OFDictData::list pl;
    for (boost::ptr_map<std::string, Patch>::const_iterator i=allPatches_.begin();
         i!=allPatches_.end(); i++)
    {
        std::vector<OFDictData::data> l = i->second->bmdEntry(pts, i->first, OFversion());
        pl.insert( pl.end(), l.begin(), l.end() );
    }
    if (OFversion()<210)
        blockMeshDict["patches"]=pl;
    else
        blockMeshDict["boundary"]=pl;

    OFDictData::list mppl;
    blockMeshDict["mergePatchPairs"]=mppl;

    if (geometries_.size()>0)
    {
        OFDictData::dict gd;
        for (const auto& geo: geometries_)
        {
            OFDictData::dict d;
            d["type"]="triSurfaceMesh";
            d["file"]="\""+geo.fileName().string()+"\"";
            gd[geo]=d;
        }
        blockMeshDict["geometry"]=gd;
    }

    if (projectedFaces_.size()>0)
    {
        OFDictData::list fd;
        for (const auto& pf: projectedFaces_)
        {
            const auto& f = pf.face();
            fd.insert(fd.end(), {
                          "project",
                          OFDictData::list({
                              pts.find(f[0])->second,
                              pts.find(f[1])->second,
                              pts.find(f[2])->second,
                              pts.find(f[3])->second
                          }),
                          pf.geometryLabel()
                      });
        }
        blockMeshDict["faces"]=fd;
    }

}


void blockMesh::writeVTK(const boost::filesystem::path& fn) const
{
  PointMap pts(allPoints_);
  numberVertices(pts);

  vtk::vtkUnstructuredGridModel m;
  double x[allPoints_.size()], y[allPoints_.size()], z[allPoints_.size()];
  int j=0;
  for (PointMap::const_iterator i=allPoints_.begin(); i!=allPoints_.end(); i++)
   {
     x[j]=i->first[0];
     y[j]=i->first[1];
     z[j]=i->first[2];
     j++;
   }
  m.setPoints(allPoints_.size(), x, y, z);

  for (boost::ptr_vector<Block>::const_iterator i=allBlocks_.begin(); i!=allBlocks_.end(); i++)
   {
     std::vector<OFDictData::data> l = i->bmdEntry(pts, OFversion());
     auto idx = boost::get<OFDictData::list&>(l[1]);
     std::vector<int> pi;
     std::transform(idx.begin(), idx.end(), std::back_inserter(pi),
                    [](const OFDictData::list::value_type& i) { return boost::get<int>(i); });
     m.appendCell( 8, pi.data(), 12 ); // 12 = HEXAHEDRON
   }

  std::ofstream os(fn.c_str());
  m.writeLegacyFile(os);
}

int blockMesh::nBlocks() const
{
  return allBlocks_.size();
}

}

}
