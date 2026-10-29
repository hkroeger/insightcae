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
#include "base/actionprogress.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/vtktools.h"
#include "base/vtklegacymodel.h"
#include "openfoam/ofdicts.h"
#include "openfoam/ofes.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

using namespace std;
using namespace boost::assign;
using namespace arma;


namespace insight {
namespace bmd {



blockMeshBlocking::blockMeshBlocking()
: scaleFactor_(1.0),
  outputScale_(1.0),
  defaultPatchName_("defaultFaces"),
  defaultPatchType_("wall"),
  allPoints_()
{}


blockMeshBlocking::blockMeshBlocking(const blockMeshBlocking &o)
  : scaleFactor_(o.scaleFactor_),
    outputScale_(o.outputScale_),
    defaultPatchName_(o.defaultPatchName_),
    defaultPatchType_(o.defaultPatchType_)
{
    copy(o);

    geometries_=o.geometries_;
    projectedVertices_=o.projectedVertices_;
    projectedFaces_=o.projectedFaces_;
    surfaceProjections_=o.surfaceProjections_;
}


void blockMeshBlocking::copy(const blockMeshBlocking& other)
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

void blockMeshBlocking::setScaleFactor(double sf)
{
  scaleFactor_=sf;
}

void blockMeshBlocking::setOutputScale(double s)
{
  insight::assertion(s>0., "output scale has to be positive");
  outputScale_=s;
}




NonOrthogonalityStatistics blockMeshBlocking::smooth(
    const SmoothingParameters& par,
    ActionProgress* progress )
{
  ActionProgressPtr ownProgress;
  if (!progress)
  {
    ownProgress = consoleProgressDisplayer.forkNewAction(1, "Block smoothing");
    progress = ownProgress.get();
  }

  // quality before smoothing at the check resolution
  NonOrthogonalityStatistics statsBefore;
  double fBefore=0.;
  int nInvalidBefore=0;
  std::vector<PointList> cornersBefore;
  boost::ptr_vector<Edge> edgesBefore;
  std::vector<ProjectedFace> projectedFacesBefore;
  std::map<Point,std::string> projectedVerticesBefore;
  if (par.checkMaxCells>0)
  {
    progress->message("evaluating the initial quality at check resolution");
    MeshPredictor chk(*this, par.checkMaxCells);
    statsBefore = chk.statistics();
    fBefore = chk.objective({}, par.p, nInvalidBefore);
    progress->message("initial (check resolution): "+statsBefore.summary());

    for (const auto& b: allBlocks_) cornersBefore.push_back(b.corners());
    for (const auto& e: allEdges_) edgesBefore.push_back(e.clone());
    projectedFacesBefore = projectedFaces_;
    projectedVerticesBefore = projectedVertices_;
  }

  progress->message("predicting the mesh at optimizer resolution");
  MeshPredictor mp(*this, par.maxCells);
  BlockMeshSmoother smoother(mp, par);
  auto stats = smoother.run(*progress);

  // vertices must stay distinct (blockMesh identifies them by coordinates)
  {
    std::set<Point> distinct;
    for (const auto& v: mp.model.vertices) distinct.insert(v.toArma());
    if (distinct.size()!=mp.model.vertices.size())
    {
      insight::Warning(
          "block smoothing moved %d vertices onto other vertices. The original blocking is kept.",
          int(mp.model.vertices.size()-distinct.size()) );
      return MeshPredictor(*this, par.checkMaxCells>0 ? par.checkMaxCells : par.maxCells).statistics();
    }
  }

  applyModel(mp.model);

  if (par.checkMaxCells>0)
  {
    progress->message("evaluating the final quality at check resolution");
    MeshPredictor chk(*this, par.checkMaxCells);
    auto statsAfter = chk.statistics();
    int nInvalidAfter=0;
    double fAfter = chk.objective({}, par.p, nInvalidAfter);
    progress->message("smoothed (check resolution): "+statsAfter.summary());

    if (nInvalidAfter>nInvalidBefore || fAfter>fBefore)
    {
      insight::Warning(
          "block smoothing did not improve the mesh at the check resolution "
          "(objective %g -> %g, invalid cells %d -> %d). The original blocking is restored. "
          "Consider increasing the optimizer resolution (maxCells).",
          fBefore, fAfter, nInvalidBefore, nInvalidAfter );

      std::map<Point, Point> oldPos;
      size_t i=0;
      for (auto& b: allBlocks_)
      {
        auto c = b.corners();
        for (size_t k=0; k<c.size(); ++k) oldPos[c[k]] = cornersBefore[i][k];
        b.setCorners(cornersBefore[i++]);
      }
      for (auto p: allPatches_)
        p.second->mapPoints(
            [&](const Point& p) { auto j=oldPos.find(p); return j==oldPos.end() ? p : j->second; } );
      projectedFaces_ = projectedFacesBefore;
      projectedVertices_ = projectedVerticesBefore;
      allEdges_.swap(edgesBefore);
      allPoints_.clear();
      for (const auto& b: allBlocks_) b.registerPoints(*this);
      for (const auto& e: allEdges_) e.registerPoints(*this);
      return statsBefore;
    }
    return statsAfter;
  }

  return stats;
}




void blockMeshBlocking::applyModel(const BlockingModel& model)
{
  std::map<Point, Point> newPos;
  for (size_t i=0; i<model.vertices.size(); ++i)
  {
    newPos[model.originalVertices[i]] = model.vertices[i].toArma();
  }

  auto mapPoint = [&](const Point& p) -> Point
  {
    auto i = newPos.find(p);
    return i==newPos.end() ? p : i->second;
  };

  for (auto& b: allBlocks_)
  {
    PointList c = b.corners();
    for (auto& p: c) p = mapPoint(p);
    b.setCorners(c);
  }

  // boundary vertices may have been moved
  for (auto p: allPatches_)
    p.second->mapPoints(mapPoint);

  for (auto& pf: projectedFaces_)
  {
    PointList f = pf.face();
    for (auto& p: f) p = mapPoint(p);
    pf = ProjectedFace(f, pf.geometryLabel());
  }

  std::map<Point,std::string> newProjectedVertices;
  for (const auto& pv: projectedVertices_)
    newProjectedVertices[mapPoint(pv.first)] = pv.second;
  projectedVertices_.swap(newProjectedVertices);

  auto splinePoints = [&](const BlockingModel::EdgeData& ed)
  {
    PointList pts;
    pts.push_back(model.vertices[ed.v0].toArma());
    for (const auto& ip: ed.intermediate)
      pts.push_back(ip.toArma());
    pts.push_back(model.vertices[ed.v1].toArma());
    return pts;
  };

  std::map<int, const BlockingModel::EdgeData*> modelEdgeOfOriginal;
  for (const auto& ed: model.edges)
  {
    if (ed.originalIndex>=0) modelEdgeOfOriginal[ed.originalIndex]=&ed;
  }

  boost::ptr_vector<Edge> newEdges;
  for (size_t ei=0; ei<allEdges_.size(); ++ei)
  {
    auto me = modelEdgeOfOriginal.find(int(ei));
    if ( me!=modelEdgeOfOriginal.end()
         && me->second->kind==BlockingModel::EdgeData::Spline )
    {
      const auto& ed = *me->second;
      if (ed.splineKeyword.empty() && ed.geometryLabel.empty())
        newEdges.push_back(new SplineEdge(splinePoints(ed)));
      else
        newEdges.push_back(new SplineEdge(splinePoints(ed), ed.splineKeyword, ed.geometryLabel));
    }
    else
    {
      newEdges.push_back(allEdges_[ei].clone());
    }
  }
  for (const auto& ed: model.edges)
  {
    if (ed.generated)
      newEdges.push_back(new SplineEdge(splinePoints(ed)));
  }
  allEdges_.swap(newEdges);

  allPoints_.clear();
  for (const auto& b: allBlocks_) b.registerPoints(*this);
  for (const auto& e: allEdges_) e.registerPoints(*this);
}

void blockMeshBlocking::setDefaultPatch(const std::string& name, std::string type)
{
  defaultPatchName_=name;
  defaultPatchType_=type;
}

void blockMeshBlocking::addGeometry(const Geometry& geo)
{
    geometries_.push_back(geo);
}

const std::vector<Geometry>& blockMeshBlocking::allGeometry() const
{
    return geometries_;
}

void blockMeshBlocking::setSurfaceProjection(const std::string& geometryLabel, const SurfaceProjection& proj)
{
    surfaceProjections_[geometryLabel]=proj;
}

void blockMeshBlocking::addProjectedVertex(const Point& pf, const std::string& geometryLabel)
{
    projectedVertices_[pf]=geometryLabel;
}

void blockMeshBlocking::addProjectedFace(const ProjectedFace& pf)
{
    projectedFaces_.push_back(pf);
}

const std::vector<ProjectedFace>& blockMeshBlocking::allProjectedFaces() const
{
    return projectedFaces_;
}

void blockMeshBlocking::removePatch(const std::string& name)
{
  std::string key(name);
  auto elem = allPatches_.find(name);
  if ( elem != allPatches_.end())
  {
    allPatches_.erase(elem);
  }
}

void blockMeshBlocking::numberVertices(PointMap& pts) const
{
  int idx=0;
  for (PointMap::iterator i=pts.begin();
       i!=pts.end(); i++)
       {
	 i->second = idx++;
  }
}

Edge &blockMeshBlocking::addEdge(Edge *edge)
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



bool blockMeshBlocking::hasEdgeBetween(const Point& p1, const Point& p2) const
{
  return edgeBetween(p1, p2)!=nullptr;
}



void blockMeshBlocking::writeVTK(const boost::filesystem::path& fn) const
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
        std::vector<OFDictData::data> l =
            i->bmdEntry(pts, OFEs::getCurrentOrPreferred().version());

        auto idx = boost::get<OFDictData::list&>(l[1]);
        std::vector<int> pi;
        std::transform(idx.begin(), idx.end(), std::back_inserter(pi),
                       [](const OFDictData::list::value_type& i) { return boost::get<int>(i); });
        m.appendCell( 8, pi.data(), 12 ); // 12 = HEXAHEDRON
    }

    std::ofstream os(fn.c_str());
    m.writeLegacyFile(os);
}

int blockMeshBlocking::nBlocks() const
{
    return allBlocks_.size();
}






blockMesh::blockMesh(OpenFOAMCase& c, ParameterSetInput ip)
  : OpenFOAMCaseElement(c, /*"blockMesh",*/ ip.forward<Parameters>()),
    blockMeshBlocking()
{}


blockMesh::blockMesh(OpenFOAMCase& c, const blockMeshBlocking &o)
    : OpenFOAMCaseElement(c, /*"blockMesh", */OpenFOAMCaseElement::Parameters() ),
    blockMeshBlocking(o)
{}


OFDictData::dict& blockMesh::getBlockMeshDict(insight::OFdicts& dictionaries) const
{
  std::string bmdLoc="constant/polyMesh/blockMeshDict";
  if (OFversion()>=500)
  {
      bmdLoc="system/blockMeshDict";
  }
  return dictionaries.lookupDict(bmdLoc);
}

namespace {

// multiply all coordinate triples (lists of three doubles) by f
void scaleCoordinateTriples(OFDictData::data& d, double f)
{
    if (auto *l = boost::get<OFDictData::list>(&d))
    {
        bool isTriple = (l->size()==3);
        for (const auto& e: *l)
            isTriple = isTriple && (boost::get<double>(&e)!=nullptr);

        if (isTriple)
        {
            for (auto& e: *l)
                boost::get<double>(e) *= f;
        }
        else
        {
            for (auto& e: *l)
                scaleCoordinateTriples(e, f);
        }
    }
}

}

void blockMesh::addIntoDictionaries(insight::OFdicts& dictionaries) const
{
    PointMap pts(allPoints_);
    numberVertices(pts);

    OFDictData::dict& blockMeshDict=getBlockMeshDict(dictionaries);

    const double os = outputScale_;
    blockMeshDict["convertToMeters"]=scaleFactor_*os;

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
        cl += p[0]/os, p[1]/os, p[2]/os;

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
            if (os!=1.)
            {
                for (auto& d: l)
                    scaleCoordinateTriples(d, 1./os);
            }
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
            if (os!=1.)
                d["scale"]=1./os; // surfaces in unscaled coordinates
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


}

}
