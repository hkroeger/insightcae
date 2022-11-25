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
#include "openfoam/openfoamdict.h"
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




typedef arma::mat Point;


typedef std::vector<Point> PointList;
typedef std::map<Point, int> PointMap;




PointList P_4(const Point& p1, const Point& p2, const Point& p3, const Point& p4);
PointList P_4(const PointList& pts, int p1, int p2, int p3, int p4);
PointList P_8(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
	      const Point& p5, const Point& p6, const Point& p7, const Point& p8);
PointList P_8_DZ(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
              const arma::mat& dz0, const arma::mat& dz1);



OFDictData::list bmdEntry(const PointList& pts, const PointMap& allPoints);



struct CS
{
    arma::mat et, eq, en;
    CS(arma::mat et, const arma::mat& en);
};


class DiscreteCurve
        : public std::vector<arma::mat>
{
public:
    CS localCS(size_t i, arma::mat en) const;
    void transform(std::function<arma::mat(const arma::mat&)> trsfFunc);
    void transform(std::function<arma::mat(const arma::mat&, const CS&, size_t)> trsfFuncLocal, const arma::mat& en);
    void interpolatedOffset(const arma::mat& ofsp0, const arma::mat& ofsp1, const arma::mat& en);
};



class Patch
{
public:
  typedef std::vector<PointList> FaceList;
  
protected:
  std::string typ_;
  FaceList faces_;
  
public:
  Patch(std::string typ="patch");
  virtual ~Patch();
  
  void addFace(Point c1, Point c2, Point c3, Point c4);
  void addFace(const PointList& corners);

  void appendPatch(const Patch& opatch);
  
  /**
   * remove all faces from current patch
   */
  void clear();
  
  Patch* transformed(const arma::mat& tm, bool inv=false, const arma::mat trans=vec3(0,0,0) ) const;
  virtual Patch* clone() const;

  inline const FaceList& faces() const { return faces_; }

  std::vector<OFDictData::data> 
  bmdEntry(const PointMap& allPoints, const std::string& name, int OFversion) const;

};




class GradingAnalyzer
{
  double grad_;
public:
  GradingAnalyzer(double grad);
  
  /**
   * compute grading from cell size at beginning and end
   */
  GradingAnalyzer(double delta0, double delta1);
  
  /**
   * grading from condition: minimum cell length delta0 on edge of length L discretized with n cells
   */
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
  virtual ~Block();

  void registerPoints(blockMesh& bmd) const;

  PointList face(const std::string& id) const;

  void swapGrad();

  std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const;

  Block* transformed(const arma::mat& tm, bool inv=false, const arma::mat trans=vec3(0,0,0)) const;
  virtual Block* clone() const;
  
  inline int nCells() const
  {
    return resolution_[0]*resolution_[1]*resolution_[2];
  }

  const PointList& corners() const;

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
    virtual ~transform2D();
  
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
    virtual ~plane2D();
    
    virtual arma::mat fwd(const arma::mat& p) const;
    virtual arma::mat rvs(const arma::mat& p) const;
};




class wedge2D
: public transform2D
{
  arma::mat fwdrot_, rvsrot_;
  
public:
    wedge2D(int idx=2);
    virtual ~wedge2D();
    
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
  virtual ~Edge();

  bool connectsPoints(const Point& c0, const Point& c1) const;
  inline const Point& c0() const { return c0_; }
  inline const Point& c1() const { return c1_; }

  virtual std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const =0;

  virtual void registerPoints(blockMesh& bmd) const;
  
  virtual Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const =0;
  virtual Edge* clone() const =0;

};



class ProjectedEdge
        : public Edge
{
    std::string geometryLabel_;
public:
  ProjectedEdge(const Point& c0, const Point& c1, const std::string& geometryLabel);

  std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const override;

  Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const override;
  Edge* clone() const override;
};




class ArcEdge
: public Edge
{
protected:
  Point midpoint_;
  
public:
  ArcEdge(const Point& c0, const Point& c1, const Point& midpoint);

  std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const override;

  Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const override;
  Edge* clone() const override;
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

    virtual Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const;
    virtual Edge* clone() const;
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

class CircularEdge_Center
: public ArcEdge
{
public:
    CircularEdge_Center
    (
      const Point& c0, const Point& c1,
      const Point& center
    );
};






class SplineEdge
: public Edge
{
protected:
  PointList intermediatepoints_;
  std::string splinekeyword_;
  std::string geometryLabel_;
  
public:
  SplineEdge(const PointList& points, 
             const std::string& splinekeyword="simpleSpline",
             const std::string& geometryLabel="");

  virtual std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const;

  virtual Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const;
  virtual Edge* clone() const;

  bmd::DiscreteCurve allPoints() const;
};





class Patch2D
: public Patch
{
protected:
  const transform2D& t2d_;
  
public:
    Patch2D(const transform2D& t2d, std::string typ="patch");   
    void addFace(const Point& c0, const Point& c1);
};


class Geometry
        : public std::string
{
    boost::filesystem::path fileName_;

public:
    Geometry(const std::string& label, const boost::filesystem::path& fileName)
        : std::string(label),
          fileName_(fileName)
    {}

    const boost::filesystem::path& fileName() const
    {
        return fileName_;
    }
};


class ProjectedFace
{
    PointList face_;
    std::string geometryLabel_;

public:
    ProjectedFace(const PointList& pts, const std::string& geometryLabel);

    const PointList& face() const;
    const std::string& geometryLabel() const;
};



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
  blockMesh(OpenFOAMCase& c, const ParameterSet& ps = ParameterSet() );

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
