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

#ifndef INSIGHT_CAD_CADFEATURE_H
#define INSIGHT_CAD_CADFEATURE_H

#include <set>
#include <map>
#include <vector>
#include <memory>

#include "base/boost_include.h"

#ifndef Q_MOC_RUN
#include <boost/spirit/include/qi.hpp>
#include "boost/functional/hash.hpp"
#include "boost/enable_shared_from_this.hpp"
#endif

#include "base/linearalgebra.h"
#include "base/exception.h"
#include "base/factory.h"

#include "occinclude.h"

#include "cadtypes.h"
#include "astbase.h"

#include "feature.h"
#include "featurefilter.h"

#include "parser.h"

namespace insight 
{
namespace cad 
{
  class Feature;
}
}

namespace boost
{
  
template<> struct hash<TopoDS_Shape>
{
  std::size_t operator()(const TopoDS_Shape& shape) const;
};

template<> struct hash<arma::mat>
{
  std::size_t operator()(const arma::mat& v) const;
};

template<> struct hash<gp_Pnt>
{
  std::size_t operator()(const gp_Pnt& v) const;
};

template<> struct hash<insight::cad::Feature>
{
  std::size_t operator()(const insight::cad::Feature& m) const;
};


}

namespace insight 
{
namespace cad 
{
  
class ParameterListHash
{
  Feature* model_;
  size_t hash_;
  
public:
  ParameterListHash();
  ParameterListHash(Feature* m);
  
  template<class T>
  void addParameter(const T& p);

  template<class T>
  void operator+=(const T& p)
  {
    addParameter(p);
  }
  
  operator size_t ();

  inline size_t getHash() const { return hash_; };
};


  
/* \mainpage
 * 
 * \section cad CAD
 * The CAD module of Insight CAE
 * 
 * \subsection cadparser CAD parser
 * Reference of the CAD parser language
 */


// class DatumPlane;
// class Feature;


std::ostream& operator<<(std::ostream& os, const Feature& m);

/**
 * Base class of all CAD modelling features
 */
class Feature
: public ASTBase,
  public boost::enable_shared_from_this<Feature>
{
  
  friend class ParameterListHash;
  
public:
  declareFactoryTable(Feature, NoParameters); 

  
//   typedef boost::shared_ptr<Feature> Ptr;
  
  struct View
  {
    TopoDS_Shape visibleEdges, hiddenEdges, crossSection;
  };
  typedef std::map<std::string, View> Views;
  
  typedef std::map<std::string, double> RefValuesList;
  typedef std::map<std::string, arma::mat> RefPointsList;
  typedef std::map<std::string, arma::mat> RefVectorsList;
  
protected :
  // needs to be unset, if this shape is used as a tool to create another shape
  mutable bool isleaf_;
  
private:
  // the shape
  // shall only be accessed via the shape() function, which triggers the build function if needed
  TopoDS_Shape shape_;
  
  FeatureSetPtr creashapes_;
  
protected:
  // all the (sub) TopoDS_Shapes in 'shape'
  TopTools_IndexedMapOfShape fmap_, emap_, vmap_, somap_, shmap_, wmap_;
  
  SubfeatureMap providedSubshapes_;
  std::map<std::string, boost::shared_ptr<Datum> > providedDatums_;
  
  RefValuesList refvalues_;
  RefPointsList refpoints_;
  RefVectorsList refvectors_;
  
  ScalarPtr density_;
  ScalarPtr areaWeight_;
  VectorPtr explicitCoG_;
  ScalarPtr explicitMass_;

  size_t hash_;
  
  TopoDS_Shape loadShapeFromFile(const boost::filesystem::path& filepath);
  void setShape(const TopoDS_Shape& shape);
  void setShapeHash();
  
  
 
public:
  declareType("Feature");
  
  Feature(const NoParameters& nop = NoParameters());
  Feature(const Feature& o);
  Feature(const TopoDS_Shape& shape);
  Feature(const boost::filesystem::path& filepath);
  Feature(FeatureSetPtr creashapes);
  virtual ~Feature();
  
  inline bool isleaf() const { return isleaf_; }
  inline void unsetLeaf() const { isleaf_=false; }
  
  inline size_t hash() const { return hash_; }
  
  virtual void setDensity(ScalarPtr rho);
  virtual double density() const;
  
  virtual void setAreaWeight(ScalarPtr rho);
  virtual double areaWeight() const;
  
  virtual double mass(double density_ovr=-1., double aw_ovr=-1.) const;
  
  virtual void build();
  
  void setMassExplicitly(ScalarPtr m);
  void setCoGExplicitly(VectorPtr cog);
  
  inline bool hasExplicitMass() const { return bool(explicitMass_); }
  inline bool hasExplicitCoG() const { return bool(explicitCoG_); }
  
  inline void unsetExplicitMass() { explicitMass_.reset(); }
  inline void unsetExplicitCoG() { explicitCoG_.reset(); }
  
  inline const std::map<std::string, boost::shared_ptr<Datum> >& providedDatums() const 
    { checkForBuildDuringAccess(); return providedDatums_; }
    
  FeaturePtr subshape(const std::string& name);
  
  inline const SubfeatureMap& providedSubshapes() const // caused failure with phx::bind!
    { checkForBuildDuringAccess(); return providedSubshapes_; }
  
  Feature& operator=(const Feature& o);
  
  bool operator==(const Feature& o) const;

  void nameFeatures();
  void extractReferenceFeatures();
  
  inline const TopoDS_Face& face(FeatureID i) const { checkForBuildDuringAccess(); return TopoDS::Face(fmap_.FindKey(i)); }
  inline const TopoDS_Edge& edge(FeatureID i) const { checkForBuildDuringAccess(); return TopoDS::Edge(emap_.FindKey(i)); }
  inline const TopoDS_Vertex& vertex(FeatureID i) const { checkForBuildDuringAccess(); return TopoDS::Vertex(vmap_.FindKey(i)); }
  inline const TopoDS_Solid& subsolid(FeatureID i) const { checkForBuildDuringAccess(); return TopoDS::Solid(somap_.FindKey(i)); }

  inline FeatureID faceID(const TopoDS_Shape& f) const { checkForBuildDuringAccess(); return fmap_.FindIndex(f); }
  inline FeatureID edgeID(const TopoDS_Shape& e) const { checkForBuildDuringAccess(); return emap_.FindIndex(e); }
  inline FeatureID vertexID(const TopoDS_Shape& v) const { checkForBuildDuringAccess(); return vmap_.FindIndex(v); }
  
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

  FeatureSetData allVerticesSet() const;
  FeatureSetData allEdgesSet() const;
  FeatureSetData allFacesSet() const;
  FeatureSetData allSolidsSet() const;

  FeatureSet allVertices() const;
  FeatureSet allEdges() const;
  FeatureSet allFaces() const;
  FeatureSet allSolids() const;
  
  FeatureSetData query_vertices(FilterPtr filter) const;
  FeatureSetData query_vertices(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSetData query_vertices_subset(const FeatureSetData& fs, FilterPtr filter) const;
  FeatureSetData query_vertices_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  
  FeatureSetData query_edges(FilterPtr filter) const;
  FeatureSetData query_edges(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSetData query_edges_subset(const FeatureSetData& fs, FilterPtr filter) const;
  FeatureSetData query_edges_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  
  FeatureSetData query_faces(FilterPtr filter) const;
  FeatureSetData query_faces(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSetData query_faces_subset(const FeatureSetData& fs, FilterPtr filter) const;
  FeatureSetData query_faces_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  
  FeatureSetData query_solids(FilterPtr filter) const;
  FeatureSetData query_solids(const std::string& queryexpr, const FeatureSetParserArgList& refs=FeatureSetParserArgList()) const;
  FeatureSetData query_solids_subset(const FeatureSetData& fs, FilterPtr filter) const;
  FeatureSetData query_solids_subset(const FeatureSetData& fs, const std::string& queryexpr, const FeatureSetParserArgList& refs) const;
  
  FeatureSet verticesOfEdge(const FeatureID& e) const;
  FeatureSet verticesOfEdges(const FeatureSet& es) const;
  
  FeatureSet verticesOfFace(const FeatureID& f) const;
  FeatureSet verticesOfFaces(const FeatureSet& fs) const;

  void saveAs(const boost::filesystem::path& filename) const;
  void exportSTL(const boost::filesystem::path& filename, double abstol) const;
  static void exportEMesh(const boost::filesystem::path& filename, const FeatureSet& fs, double abstol, double maxlen=1e10);
  
  operator const TopoDS_Shape& () const;
  const TopoDS_Shape& shape() const;
  
  View createView
  (
    const arma::mat p0,
    const arma::mat n,
    bool section,
    const arma::mat up=arma::mat()
  ) const;
  
  friend std::ostream& operator<<(std::ostream& os, const Feature& m);

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
  
  void copyDatums(const Feature& m1, const std::string& prefix="");
  void copyDatumsTransformed(const Feature& m1, const gp_Trsf& trsf, const std::string& prefix="");

  virtual const RefValuesList& getDatumScalars() const;
  virtual double getDatumScalar(const std::string& name="") const;
  virtual const RefPointsList& getDatumPoints() const;
  virtual arma::mat getDatumPoint(const std::string& name="") const;
  virtual const RefVectorsList& getDatumVectors() const;
  virtual arma::mat getDatumVector(const std::string& name="") const;
  
  virtual void write(std::ostream& file) const;
  virtual void write(const boost::filesystem::path& file) const;
  virtual void read(std::istream& file);
  virtual void read(const boost::filesystem::path& file);
};


typedef std::pair<double, arma::mat> MassAndCoG;

MassAndCoG compoundProps(const std::vector<boost::shared_ptr<Feature> >& feats, double density_ovr=-1., double aw_ovr=-1.);


template<class T>
void ParameterListHash::addParameter(const T& p)
{
  if (model_) hash_=model_->hash_;
  boost::hash_combine(hash_, p);
  if (model_) model_->hash_=hash_;
}

class SingleFaceFeature
: public Feature
{
public:
  virtual bool isSingleCloseWire() const;
  virtual TopoDS_Wire asSingleClosedWire() const;
  virtual bool isSingleFace() const;
};


class SingleVolumeFeature
: public Feature
{
public:
  virtual bool isSingleVolume() const;
};


class FeatureCache
{
  boost::filesystem::path cacheDir_;
  bool removeCacheDir_;
  
  std::set<boost::filesystem::path> usedFilesDuringRebuild_;
  
  boost::filesystem::path fileName(size_t hash) const;
  
public:
  FeatureCache(const boost::filesystem::path& cacheDir="");
  ~FeatureCache();
  
  void initRebuild();
  void finishRebuild();
  
  bool contains(size_t hash) const;
  boost::filesystem::path markAsUsed(size_t hash);
  
};

extern FeatureCache cache;

}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
