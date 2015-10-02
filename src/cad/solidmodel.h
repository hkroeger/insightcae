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
#include <memory>

#include "base/boost_include.h"

#include <boost/spirit/include/qi.hpp>

#include "base/linearalgebra.h"
#include "base/exception.h"
#include "base/factory.h"

#include "occinclude.h"

#include "cadtypes.h"

#include "feature.h"
#include "featurefilter.h"

#include "parser.h"

namespace insight 
{
namespace cad 
{
  
/* \mainpage
 * 
 * \section cad CAD
 * The CAD module of Insight CAE
 * 
 * \subsection parser CAD parser
 * Reference of the CAD parser language
 */

/** @defgroup cad_parser ISCAD Parser Language */

// class DatumPlane;
// class SolidModel;


std::ostream& operator<<(std::ostream& os, const SolidModel& m);

/**
 * Base class of all CAD modelling features
 */
class SolidModel
{
public:
  declareFactoryTable(SolidModel, NoParameters); 

  
//   typedef boost::shared_ptr<SolidModel> Ptr;
  typedef boost::spirit::qi::symbols<char, SolidModelPtr> Map;
  
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
  
  double density_, areaWeight_;
  boost::shared_ptr<arma::mat> explicitCoG_;
  boost::shared_ptr<double> explicitMass_;
  
 
public:
  declareType("SolidModel");
  
  SolidModel(const NoParameters& nop = NoParameters());
  SolidModel(const SolidModel& o);
  SolidModel(const TopoDS_Shape& shape);
  SolidModel(const boost::filesystem::path& filepath);
  virtual ~SolidModel();
  
  inline bool isleaf() const { return isleaf_; }
  inline void unsetLeaf() const { isleaf_=false; }
  
  inline void setDensity(double rho) { density_=rho; };
  inline double density() const { return density_; }
  inline void setAreaWeight(double rho) { areaWeight_=rho; };
  inline double areaWeight() const { return areaWeight_; }
  virtual double mass() const;
  
  void setMassExplicitly(double m);
  void setCoGExplicitly(const arma::mat& cog);
  
  inline bool hasExplicitMass() const { return bool(explicitMass_); }
  inline bool hasExplicitCoG() const { return bool(explicitCoG_); }
  
  inline void unsetExplicitMass() { explicitMass_.reset(); }
  inline void unsetExplicitCoG() { explicitCoG_.reset(); }
  
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

  inline FeatureID faceID(const TopoDS_Shape& f) const { return fmap_.FindIndex(f); }
  inline FeatureID edgeID(const TopoDS_Shape& e) const { return emap_.FindIndex(e); }
  inline FeatureID vertexID(const TopoDS_Shape& v) const { return vmap_.FindIndex(v); }
  
  GeomAbs_CurveType edgeType(FeatureID i) const;
  GeomAbs_SurfaceType faceType(FeatureID i) const;
  
  arma::mat vertexLocation(FeatureID i) const;
  arma::mat edgeCoG(FeatureID i) const;
  arma::mat faceCoG(FeatureID i) const;
  virtual arma::mat modelCoG() const;
  virtual double modelVolume() const;
  virtual double modelSurfaceArea() const;
  virtual double minDist(const arma::mat& p) const;
  virtual double maxVertexDist(const arma::mat& p) const;
  virtual double maxDist(const arma::mat& p) const;
  
  /**
   * return bounding box of model
   * first col: min point
   * second col: max point
   */
  arma::mat modelBndBox(double deflection=-1) const;
  
  arma::mat faceNormal(FeatureID i) const;

  FeatureSet allVertices() const;
  FeatureSet allEdges() const;
  FeatureSet allFaces() const;
  
  FeatureSet query_vertices(const FilterPtr& filter) const;
  FeatureSet query_vertices(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSet query_vertices_subset(const FeatureSet& fs, const FilterPtr& filter) const;
  FeatureSet query_vertices_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  FeatureSet query_edges(const FilterPtr& filter) const;
  FeatureSet query_edges(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSet query_edges_subset(const FeatureSet& fs, const FilterPtr& filter) const;
  FeatureSet query_edges_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  FeatureSet query_faces(const FilterPtr& filter) const;
  FeatureSet query_faces(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSet query_faces_subset(const FeatureSet& fs, const FilterPtr& filter) const;
  FeatureSet query_faces_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  
  FeatureSet verticesOfEdge(const FeatureID& e) const;
  FeatureSet verticesOfEdges(const FeatureSet& es) const;
  
  FeatureSet verticesOfFace(const FeatureID& f) const;
  FeatureSet verticesOfFaces(const FeatureSet& fs) const;

  void saveAs(const boost::filesystem::path& filename) const;
  void exportSTL(const boost::filesystem::path& filename, double abstol) const;
  static void exportEMesh(const boost::filesystem::path& filename, const FeatureSet& fs, double abstol, double maxlen=1e10);
  
  operator const TopoDS_Shape& () const;
  
  View createView
  (
    const arma::mat p0,
    const arma::mat n,
    bool section
  ) const;
  
  friend std::ostream& operator<<(std::ostream& os, const SolidModel& m);

  virtual void insertrule(parser::ISCADParser& ruleset) const;
  
  virtual bool isSingleEdge() const;
  virtual bool isSingleOpenWire() const;
  virtual bool isSingleClosedWire() const;
  virtual bool isSingleWire() const;
  virtual bool isSingleFace() const;
  virtual bool isSingleVolume() const;

  virtual TopoDS_Edge asSingleEdge() const;
  virtual TopoDS_Wire asSingleOpenWire() const;
  virtual TopoDS_Wire asSingleClosedWire() const;
  virtual TopoDS_Wire asSingleWire() const;
  virtual TopoDS_Face asSingleFace() const;
  virtual TopoDS_Shape asSingleVolume() const;

};


class Import
: public SolidModel
{
public:
  declareType("Import");
  Import(const NoParameters& nop = NoParameters());
  Import(const boost::filesystem::path& filepath);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

// =================== 0D Entities ======================
class Spring
: public SolidModel
{
public:
  declareType("Spring");
  Spring(const NoParameters& nop = NoParameters());
  Spring(const arma::mat& p0, const arma::mat& p1, double d, double winds);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual bool isSingleEdge() const { return true; };
};

// =================== 1D Primitives ======================
class SplineCurve
: public SolidModel
{
public:
  declareType("SplineCurve");
  SplineCurve(const NoParameters& nop = NoParameters());
  SplineCurve(const std::vector<arma::mat>& pts);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual bool isSingleEdge() const { return true; };
};

class Wire
: public SolidModel
{
public:
  declareType("Wire");
  Wire(const NoParameters& nop = NoParameters());
  Wire(const FeatureSet& edges);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual bool isSingleCloseWire() const;
  virtual bool isSingleOpenWire() const;
};

class Line
: public SolidModel
{
public:
  declareType("Line");
  Line(const NoParameters& nop = NoParameters());
  Line(const arma::mat& p0, const arma::mat& p1);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual bool isSingleCloseWire() const;
  virtual bool isSingleOpenWire() const;
};

class Arc
: public SolidModel
{
public:
  declareType("Arc");
  Arc(const NoParameters& nop = NoParameters());
  Arc(const arma::mat& p0, const arma::mat& p0tang, const arma::mat& p1);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual bool isSingleCloseWire() const;
  virtual bool isSingleOpenWire() const;
};

// =================== 2D Primitives ======================

class SingleFaceFeature
: public SolidModel
{
public:
  virtual bool isSingleCloseWire() const;
  virtual TopoDS_Wire asSingleClosedWire() const;
  virtual bool isSingleFace() const;
};

class Tri
: public SingleFaceFeature
{
public:
  declareType("Tri");
  Tri(const NoParameters& nop = NoParameters());
  Tri(const arma::mat& p0, const arma::mat& e1, const arma::mat& e2);
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  
};


class Quad
: public SingleFaceFeature
{
public:
  declareType("Quad");
  Quad(const NoParameters& nop = NoParameters());
  Quad(const arma::mat& p0, const arma::mat& L, const arma::mat& W);
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


class Circle
: public SingleFaceFeature
{
public:
  declareType("Circle");
  Circle(const NoParameters& nop = NoParameters());
  Circle(const arma::mat& p0, const arma::mat& n, double D);
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class RegPoly
: public SingleFaceFeature
{
public:
  declareType("RegPoly");
  RegPoly(const NoParameters& nop = NoParameters());
  RegPoly(const arma::mat& p0, const arma::mat& n, double ne, double a, 
	  const arma::mat& ez = arma::mat());
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class SplineSurface
: public SingleFaceFeature
{
public:
  declareType("SplineSurface");
  SplineSurface(const NoParameters& nop = NoParameters());
  SplineSurface(const std::vector<std::vector<arma::mat> >& pts);
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class BoundedFlatFace
: public SingleFaceFeature
{
public:
  declareType("BoundedFlatFace");
  BoundedFlatFace(const NoParameters& nop=NoParameters());
  BoundedFlatFace(const std::vector<SolidModelPtr>& edges);
  BoundedFlatFace(const std::vector<FeatureSetPtr>& edges);
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class FillingFace
: public SingleFaceFeature
{
public:
  declareType("FillingFace");
  FillingFace(const NoParameters& nop=NoParameters());
  FillingFace(const SolidModel& e1, const SolidModel& e2);
  FillingFace(const FeatureSet& e1, const FeatureSet& e2);
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

// =================== Primitives ======================

class SingleVolumeFeature
: public SolidModel
{
public:
  virtual bool isSingleVolume() const;
};

class Cylinder
: public SingleVolumeFeature
{
public:
  declareType("Cylinder");
  Cylinder(const NoParameters& nop = NoParameters());
  Cylinder(const arma::mat& p1, const arma::mat& p2, double D);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Ring
: public SingleVolumeFeature
{
public:
  declareType("Ring");
  Ring(const NoParameters& nop = NoParameters());
  Ring(const arma::mat& p1, const arma::mat& p2, double Da, double Di);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Shoulder
: public SingleVolumeFeature
{
public:
  declareType("Shoulder");
  Shoulder(const NoParameters& nop = NoParameters());
  Shoulder(const arma::mat& p0, const arma::mat& dir, double d, double Dmax);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Box
: public SingleVolumeFeature
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
  declareType("Box");
  Box(const NoParameters& nop = NoParameters());
  Box
  (
    const arma::mat& p0, 
    const arma::mat& L1, 
    const arma::mat& L2, 
    const arma::mat& L3,
    bool centered=false
  );
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Sphere
: public SingleVolumeFeature
{
public:
  declareType("Sphere");
  Sphere(const NoParameters& nop = NoParameters());
  Sphere(const arma::mat& p, double D);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
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
  declareType("Extrusion");
  Extrusion(const NoParameters& nop = NoParameters());
  Extrusion(const SolidModel& sk, const arma::mat& L, bool centered=false);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Revolution
: public SolidModel
{
public:
  declareType("Revolution");
  Revolution(const NoParameters& nop = NoParameters());
  Revolution(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double angle, bool centered=false);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Sweep
: public SolidModel
{
public:
  declareType("Sweep");
  Sweep(const NoParameters& nop = NoParameters());
  Sweep(const std::vector<SolidModelPtr>& secs);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Pipe
: public SolidModel
{
public:
  declareType("Pipe");
  Pipe(const NoParameters& nop = NoParameters());
  Pipe(const SolidModel& spine, const SolidModel& xsec);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Bar
: public SolidModel
{
public:
  declareType("Bar");
  Bar(const NoParameters& nop = NoParameters());
  
  /**
   * crate bar between p0 and p1. Cross section's xsec (single face) y-axis will be aligned with vertical direction vert.
   * bar is elongated at p0 by ext0 and at p1 by ext1, respectively.
   * 
   * @param miterangle0_vert Miter angle of bar end at (elongated) p0 around vertical direction
   * @param miterangle0_hor Miter angle of bar end at (elongated) p0 around horizontal direction
   * @param miterangle1_vert Miter angle of bar end at (elongated) p1 around vertical direction
   * @param miterangle1_hor Miter angle of bar end at (elongated) p1 around horizontal direction
   */
  Bar
  (
    const arma::mat& p0, const arma::mat& p1, 
    const SolidModel& xsec, const arma::mat& vert,
    double ext0=0.0, double ext1=0.0,
    double miterangle0_vert=0.0, double miterangle1_vert=0.0,
    double miterangle0_hor=0.0, double miterangle1_hor=0.0
  );
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Thicken
: public SolidModel
{
public:
  declareType("Thicken");
  Thicken(const NoParameters& nop = NoParameters());
  Thicken(const SolidModel& shell, double thickness, double tol=Precision::Confusion());
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class RotatedHelicalSweep
: public SolidModel
{
public:
  declareType("RotatedHelicalSweep");
  RotatedHelicalSweep(const NoParameters& nop = NoParameters());
  RotatedHelicalSweep(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double P, double revoffset=0.0);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class StitchedSolid
: public SolidModel
{
public:
  declareType("StitchedSolid");
  StitchedSolid(const NoParameters& nop = NoParameters());
  StitchedSolid(const std::vector<SolidModelPtr>& faces, double tol=1e-3);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class StitchedShell
: public SolidModel
{
public:
  declareType("StitchedShell");
  StitchedShell(const NoParameters& nop = NoParameters());
  StitchedShell(const FeatureSet& faces, double tol=1e-3);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


// =================== Boolean operations ======================
class BooleanUnion
: public SolidModel
{
public:
  declareType("BooleanUnion");
  BooleanUnion(const NoParameters& nop = NoParameters());
  BooleanUnion(const SolidModel& m);
  BooleanUnion(const SolidModel& m1, const SolidModel& m2);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

SolidModel operator|(const SolidModel& m1, const SolidModel& m2);

class BooleanSubtract
: public SolidModel
{
public:
  declareType("BooleanSubtract");
  BooleanSubtract(const NoParameters& nop = NoParameters());
  BooleanSubtract(const SolidModel& m1, const SolidModel& m2);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

SolidModel operator-(const SolidModel& m1, const SolidModel& m2);

class BooleanIntersection
: public SolidModel
{
public:
  declareType("BooleanIntersection");
  BooleanIntersection(const NoParameters& nop = NoParameters());
  BooleanIntersection(const SolidModel& m1, const SolidModel& m2);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

SolidModel operator&(const SolidModel& m1, const SolidModel& m2);

class Projected
: public SolidModel
{
public:
  declareType("Projected");
  Projected(const NoParameters& nop = NoParameters());
  Projected(const SolidModel& source, const SolidModel& target, const arma::mat& dir);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class ProjectedOutline
: public SolidModel
{
public:
  declareType("ProjectedOutline");
  ProjectedOutline(const NoParameters& nop = NoParameters());
  ProjectedOutline(const SolidModel& source, const Datum& target);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Split
: public SolidModel
{
public:
  declareType("Split");
  Split(const NoParameters& nop = NoParameters());
  Split(const SolidModel& source, const SolidModel& target);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

// =================== Cosmetic features ======================

class Fillet
: public SolidModel
{
  TopoDS_Shape makeFillets(const SolidModel& m1, const FeatureSet& edges, double r);
  
public:
  declareType("Fillet");
  Fillet(const NoParameters& nop = NoParameters());
  Fillet(const SolidModel& m1, const FeatureSet& edges, double r);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Chamfer
: public SolidModel
{
  TopoDS_Shape makeChamfers(const SolidModel& m1, const FeatureSet& edges, double l);
  
public:
  declareType("Chamfer");
  Chamfer(const NoParameters& nop = NoParameters());
  Chamfer(const SolidModel& m1, const FeatureSet& edges, double l);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


// =================== Pattern features ======================

class CircularPattern
: public SolidModel
{
  TopoDS_Shape makePattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n, bool center=false);
  
public:
  declareType("CircularPattern");
  CircularPattern(const NoParameters& nop = NoParameters());
  CircularPattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n, bool center=false);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class LinearPattern
: public SolidModel
{
  TopoDS_Shape makePattern(const SolidModel& m1, const arma::mat& axis, int n);
  
public:
  declareType("LinearPattern");
  LinearPattern(const NoParameters& nop = NoParameters());
  LinearPattern(const SolidModel& m1, const arma::mat& axis, int n);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

// =================== Modifier features ======================

class Transform
: public SolidModel
{
  TopoDS_Shape makeTransform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale=1.0);
  TopoDS_Shape makeTransform(const SolidModel& m1, const gp_Trsf& trsf);
  
public:
  declareType("Transform");
  Transform(const NoParameters& nop = NoParameters());
  Transform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale=1.0);
  Transform(const SolidModel& m1, const gp_Trsf& trsf);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Mirror
: public SolidModel
{
  
public:
  declareType("Mirror");
  Mirror(const NoParameters& nop = NoParameters());
  Mirror(const SolidModel& m1, const Datum& pl);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Place
: public SolidModel
{
  void makePlacement(const SolidModel& m, const gp_Trsf& tr);

public:
  declareType("Place");
  Place(const NoParameters& nop = NoParameters());
  Place(const SolidModel& m, const gp_Ax2& cs);
  Place(const SolidModel& m, const arma::mat& p0, const arma::mat& ex, const arma::mat& ez);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

class Compound
: public SolidModel
{
  std::vector<SolidModelPtr> components_;
  
public:
  declareType("Compound");
  Compound(const NoParameters& nop = NoParameters());
  Compound(const std::vector<SolidModelPtr>& m1);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  
  virtual arma::mat modelCoG() const;
  virtual double mass() const;
};

class Cutaway
: public SolidModel
{
public:
  declareType("Cutaway");
  Cutaway(const NoParameters& nop = NoParameters());
  Cutaway(const SolidModel& model, const arma::mat& p0, const arma::mat& n);
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};

}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
