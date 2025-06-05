#ifndef INSIGHT_GMSHCASE_H
#define INSIGHT_GMSHCASE_H

#include "cadtypes.h"
#include "base/casedirectory.h"

namespace insight {
namespace cad {


class GmshCase;

std::ostream& operator<<(std::ostream& os, GmshCase& gc);

class GmshCase
    : public std::list<std::string>
{
  friend std::ostream& operator<<(std::ostream& os, GmshCase& gc);

public:
  typedef std::map<std::string, FeatureSetPtr> NamedFeatureSet;

  enum MSHFileVersion
  {
    v10, v20, v22, v30, v40, v41
  };

  typedef std::map<std::string, int> AlgorithmList;
  static const AlgorithmList algorithms2D, algorithms3D;
  static const std::map<std::string, int> meshFormats;

private:
  CaseDirectory workDir_;

public:
  GmshCase::iterator
    endOfPreamble_,
    endOfExternalGeometryMerging_,
      endOfNamedVerticesDefinition_,
      endOfNamedEdgesDefinition_,
      endOfNamedFacesDefinition_,
      endOfNamedSolidsDefinition_,
    endOfGeometryDefinition_,
    endOfMeshingOptions_,
    endOfMeshingActions_;

protected:
  ConstFeaturePtr part_;

  int additionalPoints_;

  boost::filesystem::path executable_;

  boost::filesystem::path outputMeshFile_;
  MSHFileVersion mshFileVersion_;

  double Lmin_, Lmax_;
  int algo2D_, algo3D_;

  virtual void insertMeshingCommand();

public:
  GmshCase(
      ConstFeaturePtr part,
      const boost::filesystem::path& outputMeshFile,
      double Lmax=500., double Lmin=0.1,
//      const std::string& exeName="gmsh",
      bool keepDir=false
      );

  const boost::filesystem::path& outputMeshFile() const;

  void setAlgorithm2D(int);
  void setAlgorithm3D(int);
  void setGlobalLminLmax(double Lmin, double Lmax);

  void setMSHFileVersion(MSHFileVersion v);

  void insertLinesBefore(
      std::list<std::string>::iterator i,
      const std::vector<std::string>& lines
      );

  std::vector<std::string>
  findNamedDefinitions(const std::string& keyword) const;

  std::set<int> findNamedDefinition(const std::string& keyword, const std::string& name) const;

  std::set<int> findNamedEdges(const std::string& name) const;
  std::set<int> findNamedFaces(const std::string& name) const;
  std::set<int> findNamedSolids(const std::string& name) const;

  std::set<int> getLSDynaBeamPartIDs(const std::string& namedEdgeName) const;
  std::set<int> getLSDynaShellPartIDs(const std::string& namedFaceName) const;
  std::set<int> getLSDynaSolidPartIDs(const std::string& namedSolidName) const;

  int getUniqueLSDynaBeamPartID(const std::string& namedEdgeName) const;
  int getUniqueLSDynaShellPartID(const std::string& namedFaceName) const;

  int calcLSDynaEdgeNodeSetID(const std::string& namedEdgeName) const;

  void setLinear();
  void setQuadratic();
  void setMinimumCirclePoints(int mp);

  void nameVertices(const std::string& name, const FeatureSet& vertices);
  void nameEdges(const std::string& name, const FeatureSet& edges);
  void nameFaces(const std::string& name, const FeatureSet& faces);
  void nameSolids(const std::string& name, const FeatureSet& solids);

  void addSingleNamedVertex(const std::string& vn, const arma::mat& p);
  void setVertexLen(const std::string& vn, double L);
  void setEdgeLen(const std::string& en, double L);
  void setFaceEdgeLen(const std::string& fn, double L);

  int outputType() const;

  void doMeshing(int nthread = 1);
};


typedef std::shared_ptr<GmshCase> GmshCasePtr;

}
} // namespace insight

#endif // INSIGHT_GMSHCASE_H
