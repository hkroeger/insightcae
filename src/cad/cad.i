%module(directors="1") cad
%{
#include "solidmodel.h"
%}

namespace insight 
{
namespace cad 
{

typedef int FeatureID;
typedef std::set<FeatureID> FeatureSet;

std::ostream& operator<<(std::ostream& os, const FeatureSet& fs);


class SolidModel
{
public:
  typedef boost::shared_ptr<SolidModel> Ptr;
  
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
  
  TopoDS_Shape loadShapeFromFile(const boost::filesystem::path& filepath);
 
public:
  
  SolidModel();
  SolidModel(const SolidModel& o);
  SolidModel(const TopoDS_Shape& shape);
  SolidModel(const boost::filesystem::path& filepath);
  virtual ~SolidModel();
  
  inline bool isleaf() const { return isleaf_; }
  inline void unsetLeaf() const { isleaf_=false; }
  
  SolidModel& operator=(const SolidModel& o);

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
  
  FeatureSet query_edges(const Filter& filter) const;
  
  void saveAs(const boost::filesystem::path& filename) const;
  
  operator const TopoDS_Shape& () const;
  
  View createView
  (
    const arma::mat p0,
    const arma::mat n,
    bool section
  ) const;
  
  friend std::ostream& operator<<(std::ostream& os, const SolidModel& m);
};

}
}
