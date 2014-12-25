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

#ifndef INSIGHT_CAD_SOLIDMODEL_H
#define INSIGHT_CAD_SOLIDMODEL_H

#include <set>
#include <map>
#include <vector>

#include "boost/shared_ptr.hpp"
#include "boost/concept_check.hpp"
#include "boost/utility.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "boost/filesystem.hpp"

#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/spirit/include/qi.hpp>

#include "base/linearalgebra.h"
#include "base/exception.h"

#include "occinclude.h"

#include "feature.h"
#include "featurefilter.h"

namespace insight 
{
namespace cad 
{

class Datum;
class SolidModel;


std::ostream& operator<<(std::ostream& os, const SolidModel& m);

class SolidModel
{
public:
  typedef boost::shared_ptr<SolidModel> Ptr;
  typedef boost::spirit::qi::symbols<char, SolidModel::Ptr> Map;
  
  struct View
  {
    TopoDS_Shape visibleEdges, hiddenEdges, crossSection;
  };
  typedef std::map<std::string, View> Views;
  
protected :
  // needs to be unset, if this shape is used as a tool to create another shape
  mutable bool isleaf_;
  // the shape
  TopoDS_Shape shape_;
  // all the (sub) TopoDS_Shapes in 'shape'
  TopTools_IndexedMapOfShape fmap_, emap_, vmap_, somap_, shmap_, wmap_;
  
  SolidModel::Map providedSubshapes_;
  std::map<std::string, boost::shared_ptr<Datum> > providedDatums_;
  
  TopoDS_Shape loadShapeFromFile(const boost::filesystem::path& filepath);
  void setShape(const TopoDS_Shape& shape);
 
public:
  
  SolidModel();
  SolidModel(const SolidModel& o);
  SolidModel(const TopoDS_Shape& shape);
  SolidModel(const boost::filesystem::path& filepath);
  virtual ~SolidModel();
  
  inline bool isleaf() const { return isleaf_; }
  inline void unsetLeaf() const { isleaf_=false; }
  
  inline const std::map<std::string, boost::shared_ptr<Datum> >& providedDatums() const 
    { return providedDatums_; }
  inline SolidModel::Map providedSubshapes() // "const" caused failure with phx::bind!
    { return providedSubshapes_; }
  
  SolidModel& operator=(const SolidModel& o);
  
  bool operator==(const SolidModel& o) const;

  void nameFeatures();
  
  inline const TopoDS_Face& face(FeatureID i) const { return TopoDS::Face(fmap_.FindKey(i)); }
  inline const TopoDS_Edge& edge(FeatureID i) const { return TopoDS::Edge(emap_.FindKey(i)); }
  inline const TopoDS_Vertex& vertex(FeatureID i) const { return TopoDS::Vertex(vmap_.FindKey(i)); }
  
  GeomAbs_CurveType edgeType(FeatureID i) const;
  GeomAbs_SurfaceType faceType(FeatureID i) const;
  
  arma::mat edgeCoG(FeatureID i) const;
  arma::mat faceCoG(FeatureID i) const;
  arma::mat modelCoG() const;
  
  /**
   * return bounding box of model
   * first col: min point
   * second col: max point
   */
  arma::mat modelBndBox(double deflection=-1) const;
  
  arma::mat faceNormal(FeatureID i) const;

  FeatureSet allEdges() const;
  FeatureSet allFaces() const;
  
  FeatureSet query_edges(const FilterPtr& filter) const;
  FeatureSet query_edges(const std::string& queryexpr) const;
  FeatureSet query_edges_subset(const FeatureSet& fs, const FilterPtr& filter) const;
  FeatureSet query_faces(const FilterPtr& filter) const;
  FeatureSet query_faces(const std::string& queryexpr) const;
  FeatureSet query_faces_subset(const FeatureSet& fs, const FilterPtr& filter) const;
  
  FeatureSet verticesOfEdge(const FeatureID& e) const;
  FeatureSet verticesOfEdges(const FeatureSet& es) const;
  
  FeatureSet verticesOfFace(const FeatureID& f) const;
  FeatureSet verticesOfFaces(const FeatureSet& fs) const;

  void saveAs(const boost::filesystem::path& filename) const;
  void exportSTL(const boost::filesystem::path& filename, double abstol) const;
  
  operator const TopoDS_Shape& () const;
  
  View createView
  (
    const arma::mat p0,
    const arma::mat n,
    bool section
  ) const;
  
  friend std::ostream& operator<<(std::ostream& os, const SolidModel& m);
};


// =================== 2D Primitives ======================

class Tri
: public SolidModel
{
public:
  Tri(const arma::mat& p0, const arma::mat& e1, const arma::mat& e2);
  operator const TopoDS_Face& () const;
};


class Quad
: public SolidModel
{
public:
  Quad(const arma::mat& p0, const arma::mat& L, const arma::mat& W);
  operator const TopoDS_Face& () const;
};


class Circle
: public SolidModel
{
public:
  Circle(const arma::mat& p0, const arma::mat& n, double D);
  operator const TopoDS_Face& () const;
};

class RegPoly
: public SolidModel
{
public:
  RegPoly(const arma::mat& p0, const arma::mat& n, double ne, double a, 
	  const arma::mat& ez = arma::mat());
  operator const TopoDS_Face& () const;
};


// =================== Primitives ======================
class Cylinder
: public SolidModel
{
public:
  Cylinder(const arma::mat& p1, const arma::mat& p2, double D);
};

class Box
: public SolidModel
{
protected:
  TopoDS_Shape makeBox
  (
    const arma::mat& p0,
    const arma::mat& L1,
    const arma::mat& L2,
    const arma::mat& L3,
    bool centered=false
  );
  
public:
  Box
  (
    const arma::mat& p0, 
    const arma::mat& L1, 
    const arma::mat& L2, 
    const arma::mat& L3,
    bool centered=false
  );
};

class Sphere
: public SolidModel
{
public:
  Sphere(const arma::mat& p, double D);
};

// class Halfspace
// : public SolidModel
// {
// public:
//   Halfspace(const SolidModel& sk, const arma::mat& L, bool centered=false);
// };

class Extrusion
: public SolidModel
{
public:
  Extrusion(const SolidModel& sk, const arma::mat& L, bool centered=false);
};

class Revolution
: public SolidModel
{
public:
  Revolution(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double angle, bool centered=false);
};

class RotatedHelicalSweep
: public SolidModel
{
public:
  RotatedHelicalSweep(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double PtoD, double revoffset=0.0);
};

// =================== Boolean operations ======================
class BooleanUnion
: public SolidModel
{
public:
  BooleanUnion(const SolidModel& m1, const SolidModel& m2);
};

SolidModel operator|(const SolidModel& m1, const SolidModel& m2);

class BooleanSubtract
: public SolidModel
{
public:
  BooleanSubtract(const SolidModel& m1, const SolidModel& m2);
};

SolidModel operator-(const SolidModel& m1, const SolidModel& m2);

class BooleanIntersection
: public SolidModel
{
public:
  BooleanIntersection(const SolidModel& m1, const SolidModel& m2);
};

SolidModel operator&(const SolidModel& m1, const SolidModel& m2);

class Projected
: public SolidModel
{
public:
  Projected(const SolidModel& source, const SolidModel& target, const arma::mat& dir);
};

class Split
: public SolidModel
{
public:
  Split(const SolidModel& source, const SolidModel& target);
};

// =================== Cosmetic features ======================

class Fillet
: public SolidModel
{
  TopoDS_Shape makeFillets(const SolidModel& m1, const FeatureSet& edges, double r);
  
public:
  Fillet(const SolidModel& m1, const FeatureSet& edges, double r);
};

class Chamfer
: public SolidModel
{
  TopoDS_Shape makeChamfers(const SolidModel& m1, const FeatureSet& edges, double l);
  
public:
  Chamfer(const SolidModel& m1, const FeatureSet& edges, double l);
};


// =================== Pattern features ======================

class CircularPattern
: public SolidModel
{
  TopoDS_Shape makePattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n);
  
public:
  CircularPattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n);
};

class LinearPattern
: public SolidModel
{
  TopoDS_Shape makePattern(const SolidModel& m1, const arma::mat& axis, int n);
  
public:
  LinearPattern(const SolidModel& m1, const arma::mat& axis, int n);
};

// =================== Modifier features ======================

class Transform
: public SolidModel
{
  TopoDS_Shape makeTransform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot);
  TopoDS_Shape makeTransform(const SolidModel& m1, const gp_Trsf& trsf);
  
public:
  Transform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot);
  Transform(const SolidModel& m1, const gp_Trsf& trsf);
};

class Compound
: public SolidModel
{
  TopoDS_Shape makeCompound(const std::vector<SolidModel::Ptr>& m1);
  
public:
  Compound(const std::vector<SolidModel::Ptr>& m1);
};

}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
