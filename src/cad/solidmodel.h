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
  SolidModel(const FeatureSet& feat);
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
  inline const TopoDS_Solid& subsolid(FeatureID i) const { return TopoDS::Solid(somap_.FindKey(i)); }

  inline FeatureID faceID(const TopoDS_Shape& f) const { return fmap_.FindIndex(f); }
  inline FeatureID edgeID(const TopoDS_Shape& e) const { return emap_.FindIndex(e); }
  inline FeatureID vertexID(const TopoDS_Shape& v) const { return vmap_.FindIndex(v); }
  
  GeomAbs_CurveType edgeType(FeatureID i) const;
  GeomAbs_SurfaceType faceType(FeatureID i) const;
  
  arma::mat vertexLocation(FeatureID i) const;
  arma::mat edgeCoG(FeatureID i) const;
  arma::mat faceCoG(FeatureID i) const;
  arma::mat subsolidCoG(FeatureID i) const;
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
  FeatureSet allSolids() const;
  
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
  FeatureSet query_solids(const FilterPtr& filter) const;
  FeatureSet query_solids(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSet query_solids_subset(const FeatureSet& fs, const FilterPtr& filter) const;
  FeatureSet query_solids_subset(const FeatureSet& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  
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



class SingleFaceFeature
: public SolidModel
{
public:
  virtual bool isSingleCloseWire() const;
  virtual TopoDS_Wire asSingleClosedWire() const;
  virtual bool isSingleFace() const;
};


class SingleVolumeFeature
: public SolidModel
{
public:
  virtual bool isSingleVolume() const;
};


}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
