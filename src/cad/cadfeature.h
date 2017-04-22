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
#include "TopTools_ListOfShape.hxx"

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
  class Datum;
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

template<> struct hash<insight::cad::Datum>
{
  std::size_t operator()(const insight::cad::Datum& m) const;
};

template<> struct hash<insight::cad::Feature>
{
  std::size_t operator()(const insight::cad::Feature& m) const;
};

template<> struct hash<insight::cad::FeatureSet>
{
  std::size_t operator()(const insight::cad::FeatureSet& m) const;
};

template<> struct hash<boost::filesystem::path>
{
  std::size_t operator()(const boost::filesystem::path& fn) const;
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

struct FeatureCmdInfo
{
    std::string command_;
    std::string signature_;
    std::string documentation_;
    
    FeatureCmdInfo
    (
        std::string command,
        std::string signature,
        std::string documentation
    );
};
typedef std::vector<FeatureCmdInfo> FeatureCmdInfoList;


/**
 * Base class of all CAD modelling features
 */
class Feature
: public ASTBase,
  public boost::enable_shared_from_this<Feature>
{
  
  friend class ParameterListHash;
  
public:
  declareFactoryTableNoArgs(Feature); 

  
//   typedef boost::shared_ptr<Feature> Ptr;
  
  struct View
  {
    TopoDS_Shape visibleEdges, hiddenEdges;
    boost::shared_ptr<TopTools_ListOfShape> crossSections;
    /**
     * 2D size of projection
     */
    double width, height;
    /**
     * center of features
     */
    double drawing_ctr_x=0, drawing_ctr_y=0;
    /**
     * placement on drawing
     */
    double insert_x=0, insert_y=0;
  };
  typedef std::map<std::string, View> Views;
  
  typedef std::map<std::string, double> RefValuesList;
  typedef std::map<std::string, arma::mat> RefPointsList;
  typedef std::map<std::string, arma::mat> RefVectorsList;
  
protected :
  // needs to be unset, if this shape is used as a tool to create another shape
  mutable bool isleaf_;
  
  mutable boost::shared_ptr<GProp_GProps> volprops_;
  
private:
  // the shape
  // shall only be accessed via the shape() function, which triggers the build function if needed
  TopoDS_Shape shape_;
  
  FeatureSetPtr creashapes_;
  
protected:
  // all the (sub) TopoDS_Shapes in 'shape'
  TopTools_IndexedMapOfShape fmap_, emap_, vmap_, somap_, shmap_, wmap_;
  
  SubfeatureMap providedSubshapes_;
  FeatureSetPtrMap providedFeatureSets_;
  DatumPtrMap providedDatums_;
  
  RefValuesList refvalues_;
  RefPointsList refpoints_;
  RefVectorsList refvectors_;
  
  ScalarPtr visresolution_;
  ScalarPtr density_;
  ScalarPtr areaWeight_;

  size_t hash_;
  
  void updateVolProps() const;
  void setShape(const TopoDS_Shape& shape);
  
  void setShapeHash();
  
  /**
   * sets the hash from input parameters
   * (not the shape)
   * has to be computed before build!
   */
  virtual void calcHash();
  
  void loadShapeFromFile(const boost::filesystem::path& filepath);
  
 
public:
  declareType("Feature");
  
  Feature();
  Feature(const Feature& o);
  Feature(const TopoDS_Shape& shape);
//   Feature(const boost::filesystem::path& filepath);
  Feature(FeatureSetPtr creashapes);
  
  static FeaturePtr CreateFromFile(const boost::filesystem::path& filepath);
  
  virtual ~Feature();
  
  inline bool isleaf() const { return isleaf_; }
  inline void unsetLeaf() const { isleaf_=false; }
  
  inline size_t hash() const { return hash_; }
  
  inline void setVisResolution( ScalarPtr r ) { visresolution_=r; }
  virtual void setDensity(ScalarPtr rho);
  virtual double density() const;
  
  virtual void setAreaWeight(ScalarPtr rho);
  virtual double areaWeight() const;
  
  virtual double mass(double density_ovr=-1., double aw_ovr=-1.) const;
  
  virtual void build();
    
  inline const DatumPtrMap& providedDatums() const 
    { checkForBuildDuringAccess(); return providedDatums_; }
    
  FeaturePtr subshape(const std::string& name);
  FeatureSetPtr providedFeatureSet(const std::string& name);
  
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

  inline FeatureID solidID(const TopoDS_Shape& f) const { checkForBuildDuringAccess(); int i=somap_.FindIndex(f); if (i==0) throw insight::Exception("requested solid not indexed!"); return i; }
  inline FeatureID faceID(const TopoDS_Shape& f) const { checkForBuildDuringAccess(); int i=fmap_.FindIndex(f); if (i==0) throw insight::Exception("requested face not indexed!"); return i; }
  inline FeatureID edgeID(const TopoDS_Shape& e) const { checkForBuildDuringAccess(); int i=emap_.FindIndex(e); if (i==0) throw insight::Exception("requested edge not indexed!"); return i; }
  inline FeatureID vertexID(const TopoDS_Shape& v) const { checkForBuildDuringAccess(); int i=vmap_.FindIndex(v); if (i==0) throw insight::Exception("requested vertex not indexed!"); return i; }
  
  GeomAbs_CurveType edgeType(FeatureID i) const;
  GeomAbs_SurfaceType faceType(FeatureID i) const;
  
  arma::mat vertexLocation(FeatureID i) const;
  arma::mat edgeCoG(FeatureID i) const;
  arma::mat faceCoG(FeatureID i) const;
  arma::mat subsolidCoG(FeatureID i) const;
  double subsolidVolume(FeatureID i) const;
  
  /**
   * returns the center of gravity of the shape volume
   */
  virtual arma::mat modelCoG(double density_ovr=-1.) const;
  
  /**
   * returns the center of gravity of the shape surface
   */
  virtual arma::mat surfaceCoG(double areaWeight_ovr=-1.) const;

  /**
   * returns selected axes of inertia of the model surface only
   */
  virtual arma::mat surfaceInertia(int axis=0) const;
  
  virtual double modelVolume() const;
  virtual double modelSurfaceArea() const;
  virtual double minDist(const arma::mat& p) const;
  virtual double maxVertexDist(const arma::mat& p) const;
  virtual double maxDist(const arma::mat& p) const;

  /**
   * return  the inertia tensor with respect to the current CS and the global origin
   */
  virtual arma::mat modelInertia(double density_ovr=-1.) const;
  
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

  void saveAs
  (
    const boost::filesystem::path& filename,
    const std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> >& namedfeats 
      = std::vector<boost::fusion::vector2<std::string, FeatureSetPtr> >()
  ) const;
  
  void exportSTL(const boost::filesystem::path& filename, double abstol) const;
  static void exportEMesh(const boost::filesystem::path& filename, const FeatureSet& fs, double abstol, double maxlen=1e10);
  
  operator const TopoDS_Shape& () const;
  const TopoDS_Shape& shape() const;
  
  View createView
  (
    const arma::mat p0,
    const arma::mat n,
    bool section,
    const arma::mat up=arma::mat(),
    bool poly=false,
    bool skiphl=false
  ) const;
  
  friend std::ostream& operator<<(std::ostream& os, const Feature& m);

  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual FeatureCmdInfoList ruleDocumentation() const;
  
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
  
  void copyDatums(const Feature& m1, const std::string& prefix="", std::set<std::string> excludes = std::set<std::string>() );
  void copyDatumsTransformed(const Feature& m1, const gp_Trsf& trsf, const std::string& prefix="", std::set<std::string> excludes = std::set<std::string>() );

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
  
  /**
   * whether this feature does not create or alters geometry but only transforms it.
   * Return false e.g. four compound or transform
   */
  virtual bool isTransformationFeature() const;
  virtual gp_Trsf transformation() const;
};


typedef boost::fusion::vector3<double, arma::mat, arma::mat> Mass_CoG_Inertia;

Mass_CoG_Inertia compoundProps(const std::vector<boost::shared_ptr<Feature> >& feats, double density_ovr=-1., double aw_ovr=-1.);

arma::mat rotTrsf(const gp_Trsf& tr);
arma::mat transTrsf(const gp_Trsf& tr);


template<class T>
void ParameterListHash::addParameter(const T& p)
{
  boost::hash_combine(hash_, p);
  
  // update
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


// class FeatureCache
// {
//   boost::filesystem::path cacheDir_;
//   bool removeCacheDir_;
//   
//   std::set<boost::filesystem::path> usedFilesDuringRebuild_;
//   
//   boost::filesystem::path fileName(size_t hash) const;
//   
// public:
//   FeatureCache(const boost::filesystem::path& cacheDir="");
//   ~FeatureCache();
//   
//   void initRebuild();
//   void finishRebuild();
//   
//   bool contains(size_t hash) const;
//   boost::filesystem::path markAsUsed(size_t hash);
//   
// };


// #warning cachable feature always have to be stored in shared_ptrs! create functions and private constructors should be issues to ensure this.
class FeatureCache
: public std::map<size_t, FeaturePtr>
{
  std::set<size_t> usedDuringRebuild_;
  
public:
  FeatureCache();
  ~FeatureCache();
  
  void initRebuild();
  void finishRebuild();
  
  void insert(FeaturePtr p);
  bool contains(size_t hash) const;
  
  template<class T>
  boost::shared_ptr<T> markAsUsed(size_t hash)
  {
    iterator i=this->find(hash);
    
    if (i==end()) 
      throw insight::Exception
      (
	"requested entry in CAD feature cache is not found!"
      );
    
    boost::shared_ptr<T> cp
    ( 
      boost::dynamic_pointer_cast<T>( i->second ) 
    );
    
    if (!cp) 
      throw insight::Exception
      (
	"requested entry in CAD feature cache found,"
	" but is of wrong type! (cache: "+i->second->type()
      );
    
    usedDuringRebuild_.insert(hash);
    return cp;
  }
  
};

extern FeatureCache cache;

}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
