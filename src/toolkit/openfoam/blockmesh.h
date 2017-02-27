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

#include "openfoam/openfoamcase.h"
#include "base/linearalgebra.h"

#include "boost/variant.hpp"
#include "boost/assign/list_of.hpp"
#include <boost/assign/std/vector.hpp>

#include <cmath>
#include <vector>
#include <map>
#include <armadillo>

namespace insight {
  
namespace bmd
{

class blockMesh;

/*
class Point
: public arma::mat
{
  
public:
  Point();
  Point(const arma::mat& m);
  Point(double x, double y, double z);*/
/*
    def __str__(self):
        return "(%f %f %f)\n" % (self.x, self.y, self.z)

    def bmdStr(self, pointOffsets):
        return "(%f %f %f)\n" % (
            self.x+pointOffsets[self][0], 
            self.y+pointOffsets[self][1], 
            self.z+pointOffsets[self][2])

    def __hash__(self):
        s=str(self)
        return hash(s)

    def __cmp__(self, other):
        return cmp(hash(self), hash(other))
*//*
  void operator=(const arma::mat& m);
};*/

typedef arma::mat Point;



typedef bool (*Comp)(const Point& v1, const Point& v2);
bool compare(const Point& v1, const Point& v2);

typedef std::vector<Point> PointList;
typedef std::map<Point, int, Comp> PointMap;

PointList P_4(const Point& p1, const Point& p2, const Point& p3, const Point& p4);
PointList P_4(const PointList& pts, int p1, int p2, int p3, int p4);
PointList P_8(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
	      const Point& p5, const Point& p6, const Point& p7, const Point& p8);


class Patch
{
public:
  typedef std::vector<PointList> FaceList;
  
protected:
  std::string typ_;
  FaceList faces_;
  
public:
  Patch(std::string typ="patch");
  
  void addFace(Point c1, Point c2, Point c3, Point c4);
  void addFace(const PointList& corners);

  void appendPatch(const Patch& opatch);
  
  /**
   * remove all faces from current patch
   */
  void clear();
  
  Patch* transformed(const arma::mat& tm, bool inv=false) const;

  std::vector<OFDictData::data> 
  bmdEntry(const PointMap& allPoints, const std::string& name, int OFversion) const;
};

class GradingAnalyzer
{
  double grad_;
public:
  GradingAnalyzer(double grad);
  GradingAnalyzer(double delta0, double delta1);
  GradingAnalyzer(double delta0, double L, int n);
  
  inline double grad() const { return grad_; }
  
  int calc_n(double delta0, double L) const;
  double calc_L(double delta0, int n) const;
  double calc_delta1(double delta0) const;
};

class Block
{
public:
  typedef boost::variant<
      double, 
      std::vector<double> 
      > Grading;
  typedef std::vector<Grading> GradingList;
        
protected:
  PointList corners_;
  std::vector<int> resolution_;
  GradingList grading_;
  std::string zone_;
  bool inv_;
  
public:
  Block(PointList corners, 
	int resx, int resy, int resz, 
	GradingList grading = GradingList(3, 1), 
	std::string zone="", 
	bool inv=false);

  void registerPoints(blockMesh& bmd) const;

  PointList face(const std::string& id) const;

  void swapGrad();

  std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const;

  Block* transformed(const arma::mat& tm, bool inv=false) const;
  
  inline int nCells() const
  {
    return resolution_[0]*resolution_[1]*resolution_[2];
  }

};


class transform2D
{
protected:
  int idx_;
  std::vector<int> map_;
  std::vector<int> dir_;
  
  Patch fwdPatch_, rvsPatch_;
  
public:
    transform2D(int idx=2);
  
    arma::mat mapped3D(const arma::mat& p) const;
    
    virtual arma::mat fwd(const arma::mat& p) const =0;
    virtual arma::mat rvs(const arma::mat& p) const =0;

    inline Patch& fwdPatch() { return fwdPatch_; }
    inline Patch& rvsPatch() { return rvsPatch_; }

    void addFwdRvsPatches(blockMesh *bmd);
};

class plane2D
: public transform2D
{
protected:
  arma::mat ofs_;
  
public:
    plane2D(double thick, int idx=2); 
    virtual arma::mat fwd(const arma::mat& p) const;
    virtual arma::mat rvs(const arma::mat& p) const;
};


class wedge2D
: public transform2D
{
  arma::mat fwdrot_, rvsrot_;
  
public:
    wedge2D(int idx=2);
    virtual arma::mat fwd(const arma::mat& p) const;
    virtual arma::mat rvs(const arma::mat& p) const;
};


class Block2D
: public Block
{
protected:
  transform2D& t2d_;
  
public:
    Block2D
    (
      transform2D& t2d, 
      PointList corners, 
      int resx, int resy, 
      GradingList grading = GradingList(3, 1), 
      std::string zone="", 
      bool inv=false
    );
};

class Edge
{
protected:
  Point c0_, c1_;
  
public:
  Edge(const Point& c0, const Point& c1);
  
  virtual std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const =0;

  virtual void registerPoints(blockMesh& bmd) const;
  
  virtual Edge* transformed(const arma::mat& tm) const =0;

};

class ArcEdge
: public Edge
{
protected:
  Point midpoint_;
  
public:
  ArcEdge(const Point& c0, const Point& c1, const Point& midpoint);

  virtual std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const;

  virtual Edge* transformed(const arma::mat& tm) const;
};


class EllipseEdge
: public Edge
{
protected:
  Point center_;
  double ex_;

public:
    EllipseEdge
    (
      const Point& c0, const Point& c1, 
      const Point& center,
      double ex
    );
    
    virtual std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const;

    virtual Edge* transformed(const arma::mat& tm) const;
};


class CircularEdge
: public ArcEdge
{
public:
    CircularEdge
    (
      const Point& c0, const Point& c1, 
      Point startPoint=vec3(0,0,0),
      arma::mat axis=vec3(0,0,1)
    );
};

/*
class GenArcEdge
: public ArcEdge
{
    def __init__(self, corners, R, axis, startPoint=array([0,0,0])):
        from scipy.optimize.nonlin import broyden2
        a=axis
        a/=sqrt(dot(a,a))
        def F(x):
            M=array([x[0], x[1], x[2]])
            r1=corners[0]-M
            r2=corners[1]-M
            cr=cross(r1, r2)
            cr/=sqrt(dot(cr, cr))
            return [
                1.0-dot(a, cr),
                sqrt(dot(r1, r1))-R,
                sqrt(dot(r2, r2))-R
                 ]
        startPoint=array(
            broyden2(F, list(startPoint), 20, verbose=True)
            )
        print corners, startPoint
        bp=startPoint+axis*dot(axis, (corners[0]-startPoint))
        r1=corners[0]-bp
        r=sqrt(dot(r1, r1))
        print R, r
        mp0=0.5*((corners[0]-bp)+(corners[1]-bp))
        mp0/=sqrt(dot(mp0,mp0))
        bmdArcEdge.__init__(self, corners, bp+r*mp0)
};
*/

class SplineEdge
: public Edge
{
protected:
  PointList intermediatepoints_;
  std::string splinekeyword_;
  
public:
  SplineEdge(const PointList& points, 
             std::string splinekeyword="simpleSpline");

  virtual std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const;

  virtual Edge* transformed(const arma::mat& tm) const;
};

/*
def splineEdgeAlongCurve(curve, p0, p1, 
                         start0=0.5, start1=0.5, 
                         lim=False, fixed=False):
    if not fixed:
        t0=Curves.tNearP(curve, p0, start0, limited=lim)
        t1=Curves.tNearP(curve, p1, start1, limited=lim)
    else:
        t0=start0
        t1=start1
    print "Generating spline on curve from param ", t0, " to ", t1
    if (fabs(t0-t1)<1e-4):
        raise Exception('start and end of spline are the same point')
    allEdges.append(bmdSplineEdge([p0, p1], pointsInRange(curve, t0, t1)))
    return t0, t1
*/



class Patch2D
: public Patch
{
protected:
  const transform2D& t2d_;
  
public:
    Patch2D(const transform2D& t2d, std::string typ="patch");   
    void addFace(const Point& c0, const Point& c1);
};

/*
class ArcEdge2D
{
    def __init__(self, t2d, corners, midpoint):
        self.t2d=t2d
        self.corners=corners
        self.midpoint=midpoint

    def register(self, bmd):
        for c in self.corners:
            bmd.addPoint(Point(self.t2d.fwd(c)))
            bmd.addPoint(Point(self.t2d.rvs(c)))

    def bmdEntry(self, allPoints):
        mp=self.t2d.fwd(self.midpoint)
        s="arc %d %d (%f %f %f)\n" % (
            allPoints[Point(self.t2d.fwd(self.corners[0]))], 
            allPoints[Point(self.t2d.fwd(self.corners[1]))],
            mp[0], mp[1], mp[2])
        mp=self.t2d.rvs(self.midpoint)
        s+="arc %d %d (%f %f %f)\n" % (
            allPoints[Point(self.t2d.rvs(self.corners[0]))], 
            allPoints[Point(self.t2d.rvs(self.corners[1]))],
            mp[0], mp[1], mp[2])
        return s
};

class SplineEdge2D
{
    def __init__(self, t2d, corners, intermediatepoints, 
                 splinekeyword="GSLSpline"):
        self.t2d=t2d
        self.corners=corners
        self.intermediatepoints=intermediatepoints
        self.skey=splinekeyword

    def register(self, bmd):
        for c in self.corners:
            bmd.addPoint(Point(self.t2d.fwd(c)))
            bmd.addPoint(Point(self.t2d.rvs(c)))
            
    def bmdEntry(self, allPoints):
        s="%s %d %d\n(\n" % (
            self.skey,
            allPoints[Point(self.t2d.fwd(self.corners[0]))], 
            allPoints[Point(self.t2d.fwd(self.corners[1]))])
        for p in self.intermediatepoints:
            s+="(%s)\n"%(" ".join(["%g"%c for c in self.t2d.fwd(p)]))
        s+=")\n"

        s+="%s %d %d\n(\n" % (
            self.skey,
            allPoints[Point(self.t2d.rvs(self.corners[0]))], 
            allPoints[Point(self.t2d.rvs(self.corners[1]))])
        for p in self.intermediatepoints:
            s+="(%s)\n"%(" ".join(["%g"%c for c in self.t2d.rvs(p)]))
        s+=")\n"
        
        return s
};
*/

class blockMesh 
: public insight::OpenFOAMCaseElement
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
  
public:
  blockMesh(OpenFOAMCase& c);
  
  void setScaleFactor(double sf);
  void setDefaultPatch(const std::string& name, std::string type="patch");
  
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
  
  inline Edge& addEdge(Edge *edge) 
  { 
    edge->registerPoints(*this);
    allEdges_.push_back(edge);
    return *edge;
  }
  
  inline Patch& addPatch(const std::string& name, Patch *patch) 
  { 
    if (name=="")
      throw insight::Exception("Empty patch names are not allowed!");
    
    std::string key(name);
    allPatches_.insert(key, patch); 
    return *patch; 
  }
  
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
  
  virtual void addIntoDictionaries(insight::OFdicts& dictionaries) const;
  
  static std::string category() { return "Meshing"; }
};

typedef boost::shared_ptr<blockMesh> blockMeshPtr;

}

}

#endif // INSIGHT_BLOCKMESH_H
