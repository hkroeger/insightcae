#include "gmshcase.h"

#include "base/exception.h"
#include "boost/process.hpp"
#include "base/softwareenvironment.h"

#include "cadfeature.h"

namespace insight {
namespace cad {



const std::map<std::string, int> GmshCase::algorithms2D =
{
  {"MeshAdapt", 1},
  {"Automatic", 2},
  {"Delaunay", 5},
  {"Frontal-Delaunay", 6},
  {"BAMG", 7},
  {"DelQuad", 8}
};




const std::map<std::string, int> GmshCase::algorithms3D =
{
  {"Delaunay", 1},
  {"Frontal", 4},
  {"MMG3D", 7},
  {"R-Tree", 9}
};


const std::map<std::string, int> GmshCase::meshFormats =
{
    { ".stl", 27 },
    { ".msh", 1 },
    { ".unv", 2 },
    { ".med", 33 },
    { ".vtk", 16 },
    { ".key", 51 },
    { ".vrml", 19},
    { ".mail", 21},
    { ".pos", 26},
    { ".p3d", 28},
    { ".mesh", 30},
    { ".bdf", 31},
    { ".cgns", 32},
    { ".diff", 34},
    { ".ir3", 38},
    { ".inp", 39},
    { ".ply2", 40},
    { ".celum", 41},
    { ".su2", 42},
    { ".tochnog", 47},
    { ".neu", 49},
    { ".matlab", 50}
};


void GmshCase::insertLinesBefore(
        GmshCase::iterator i,
        const std::vector<std::string> &lines)
{
  for (auto j=lines.begin(); j!=lines.end(); ++j)
  {
    insert(i, *j);
  }
}

std::vector<std::string>
GmshCase::findNamedDefinitions(const std::string &keyword) const
{
    std::vector<std::string> result;

    boost::regex re(keyword+" *\\(\"(.*)\"\\) *= *{(.*)}");
    for (const auto& l: *this)
    {
        boost::smatch m;
        if (regex_search(l, m, re))
        {
            result.push_back(m[1]);
        }
    }

    return result;
}


std::set<int> GmshCase::findNamedDefinition(const std::string &keyword, const std::string &name) const
{
    std::set<int> result;

    boost::regex re(keyword+" *\\(\"(.*)\"\\) *= *{(.*)}");
    for (const auto& l: *this)
    {
        boost::smatch m;
        if (regex_search(l, m, re))
        {
            if (m[1]==name)
            {
                std::vector<std::string> nums;
                boost::split(nums, m[2], boost::is_any_of(","));
                std::transform(nums.begin(), nums.end(),
                               std::inserter(result, result.begin()),
                               [](const std::string& n)
                               { return boost::lexical_cast<int>(n); });
                return result;
            }
        }
    }

    return result;
}


GmshCase::GmshCase(
    insight::cad::ConstFeaturePtr part,
    const boost::filesystem::path& outputMeshFile,
    double Lmax, double Lmin,
    bool keepDir
    )
: workDir_(keepDir),
  part_(part),
  additionalPoints_(0),
  outputMeshFile_(outputMeshFile),
  mshFileVersion_(v41),
  algo2D_(1), algo3D_(4)
{
  setGlobalLminLmax(Lmin, Lmax);

  // prefer gmsh from insightcae dependency package
  executable_ = boost::process::search_path("gmshinsightcae");
  if (executable_.empty())
  {
    executable_ = boost::process::search_path("gmsh");
  }
  if (executable_.empty())
  {
    throw insight::Exception("Could not find executable \"gmsh\" in PATH! Please check, if it is installed correctly,");
  }

  push_back("SetFactory(\"OpenCASCADE\")");
  push_back("// Preamble");
  push_back("");
  endOfPreamble_ = --end();
  push_back("// Merging external geometry");
  push_back("");
  endOfExternalGeometryMerging_ = --end();
  push_back("// Geometry definitions");
    push_back("");
    endOfNamedVerticesDefinition_= --end();
    push_back("");
    endOfNamedEdgesDefinition_= --end();
    push_back("");
    endOfNamedFacesDefinition_= --end();
    push_back("");
    endOfNamedSolidsDefinition_= --end();
  push_back("");
  endOfGeometryDefinition_ = --end();
  push_back("// Meshing options");
  push_back("");
  endOfMeshingOptions_  = --end();
  push_back("// Meshing actions");
  push_back("");
  endOfMeshingActions_ = --end();
  push_back("// End");

  insertLinesBefore(endOfPreamble_, {
    "Geometry.AutoCoherence = 0",
    "Geometry.OCCSewFaces = 0",
    "Geometry.Tolerance = 1e-10"
  });

  boost::filesystem::path geomFile = workDir_ / (outputMeshFile_.stem().string() + ".brep");

  part_->saveAs(geomFile.string());

  insertLinesBefore(endOfExternalGeometryMerging_, {
    "Merge \""+boost::filesystem::absolute(geomFile).string()+"\""
                    });
}

const boost::filesystem::path &GmshCase::outputMeshFile() const
{
    return outputMeshFile_;
}

void GmshCase::setAlgorithm2D(int a)
{
  algo2D_=a;
}


void GmshCase::setAlgorithm3D(int a)
{
  algo3D_=a;
}


void GmshCase::setGlobalLminLmax(double Lmin, double Lmax)
{
  Lmin_=Lmin;
  Lmax_=Lmax;
}


void GmshCase::setMSHFileVersion(GmshCase::MSHFileVersion v)
{
  mshFileVersion_=v;
}


void GmshCase::setLinear()
{
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.ElementOrder=1"
  });
}


void GmshCase::setQuadratic()
{
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.ElementOrder=2",
    "Mesh.SecondOrderLinear=0"
  });
}

void GmshCase::setMinimumCirclePoints(int mp)
{
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.CharacteristicLengthFromCurvature=1",
    "Mesh.MinimumCirclePoints="+boost::lexical_cast<std::string>(mp)
  });
}



std::set<int> GmshCase::findNamedEdges(const std::string &name) const
{
  return findNamedDefinition("Physical Line", name);
}

std::set<int> GmshCase::findNamedFaces(const std::string &name) const
{
  return findNamedDefinition("Physical Surface", name);
}

std::set<int> GmshCase::findNamedSolids(const std::string &name) const
{
    return findNamedDefinition("Physical Volume", name);
}

std::set<int> GmshCase::getLSDynaBeamPartIDs(const std::string &namedEdgeName) const
{
    auto eIDs = findNamedEdges(namedEdgeName);

    std::set<int> partIDs;
    std::transform(
        eIDs.begin(), eIDs.end(),
        std::inserter(partIDs, partIDs.begin()),
        [](int eID) { return 1*1000000+eID; } );
    return partIDs;
}

std::set<int> GmshCase::getLSDynaShellPartIDs(const std::string &namedFaceName) const
{
    auto fIDs = findNamedFaces(namedFaceName);

    std::set<int> partIDs;
    std::transform(
                fIDs.begin(), fIDs.end(),
                std::inserter(partIDs, partIDs.begin()),
                [](int fID) { return 2*1000000+fID; } );
    return partIDs;
}


std::set<int> GmshCase::getLSDynaSolidPartIDs(const std::string &namedSolidName) const
{
    auto sIDs = findNamedSolids(namedSolidName);

    std::set<int> partIDs;
    std::transform(
                sIDs.begin(), sIDs.end(),
                std::inserter(partIDs, partIDs.begin()),
                [](int sID) { return 3*1000000+sID; } );
    return partIDs;
}

int GmshCase::getUniqueLSDynaBeamPartID(const std::string &namedEdgeName) const
{
    auto ids=getLSDynaBeamPartIDs(namedEdgeName);
    insight::assertion(
        ids.size()==1,
        "expected a single beam part, found %d", ids.size()
        );
    return *ids.begin();
}

int GmshCase::getUniqueLSDynaShellPartID(const std::string &namedFaceName) const
{
    auto ids=getLSDynaShellPartIDs(namedFaceName);
    insight::assertion(
        ids.size()==1,
        "expected a single shell part, found %d", ids.size()
        );
    return *ids.begin();
}

int GmshCase::calcLSDynaEdgeNodeSetID(const std::string &namedEdgeName) const
{
    int i0 = findNamedDefinitions("Physical Point").size();
    auto nl = findNamedDefinitions("Physical Line");
    auto nei = std::find(nl.begin(), nl.end(), namedEdgeName);
    insight::assertion(
        nei!=nl.end(),
        "named edge %s not found", namedEdgeName.c_str() );
    return i0+std::distance(nl.begin(), nei);
}

void GmshCase::nameVertices(const std::string& name, const FeatureSet& vertices)
{
    insight::assertion(
                vertices.shape()==cad::Vertex,
                "expected vertex feature set" );

  std::vector<std::string> nums;

  std::transform(
              vertices.data().begin(),
              vertices.data().end(),
              std::back_inserter(nums),
              [](int i)
                { return boost::lexical_cast<std::string>(i); }
  );

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Point(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}


void GmshCase::nameEdges(const std::string& name, const FeatureSet& edges)
{
    insight::assertion(
                edges.shape()==cad::Edge,
                "expected edge feature set" );

  std::vector<std::string> nums;

  std::transform(
              edges.data().begin(),
              edges.data().end(),
              std::back_inserter(nums),
              [](int i)
                { return boost::lexical_cast<std::string>(i); }
  );

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Line(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}

void GmshCase::nameFaces(const std::string& name, const FeatureSet& faces)
{
    insight::assertion(
                faces.shape()==cad::Face,
                "expected face feature set" );

  std::vector<std::string> nums;

  std::transform(
              faces.data().begin(),
              faces.data().end(),
              std::back_inserter(nums),
              [](int i)
                { return boost::lexical_cast<std::string>(i); }
  );

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Surface(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}

void GmshCase::nameSolids(const std::string& name, const FeatureSet& solids)
{
    insight::assertion(
                solids.shape()==cad::Solid,
                "expected solid feature set" );

    std::vector<std::string> nums;

  std::transform(
              solids.data().begin(),
              solids.data().end(),
              std::back_inserter(nums),
              [](int i)
                { return boost::lexical_cast<std::string>(i); }
  );

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Volume(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}

void GmshCase::addSingleNamedVertex(const std::string& vn, const arma::mat& p)
{
  additionalPoints_++;
  int id=part_->allVertices()->data().size()+additionalPoints_;
  insertLinesBefore(endOfGeometryDefinition_, {
    str( boost::format("Point(%d) = {%g, %g, %g, 999};")%id%p(0)%p(1)%(p(2)) ),
    str( boost::format("Physical Point(\"%s\") = {%d};")%vn%id )
  });
}




void GmshCase::setVertexLen(const std::string& vn, double L)
{
  insertLinesBefore(endOfGeometryDefinition_, {
    "Characteristic Length{\""+vn+"\"}="+boost::lexical_cast<std::string>(L)
  });
}




void GmshCase::setEdgeLen(const std::string& en, double L)
{
  FeatureSet fs(part_, cad::Edge);
  fs.setData( findNamedEdges(en) );
  FeatureSet vs = part_->verticesOfEdges(fs);

  std::vector<std::string> nums;
  std::transform(vs.data().begin(), vs.data().end(),
                 std::back_inserter(nums),
                 [](int i) { return boost::lexical_cast<std::string>(i); });

  insertLinesBefore(endOfGeometryDefinition_, {
    "Characteristic Length{"+boost::join(nums, ",")+"}="
                        +boost::lexical_cast<std::string>(L)
                    });
}




void GmshCase::setFaceEdgeLen(const std::string& fn, double L)
{
  FeatureSet fs(part_, cad::Face);
  fs.setData( findNamedFaces(fn) );
  FeatureSet vs = part_->verticesOfFaces(fs);

  std::vector<std::string> nums;
  std::transform(vs.data().begin(), vs.data().end(),
                 std::back_inserter(nums),
                 [](int i) { return boost::lexical_cast<std::string>(i); });

  insertLinesBefore(endOfGeometryDefinition_, {
    "Characteristic Length{"+boost::join(nums, ",")+"}="+boost::lexical_cast<std::string>(L)
                    });
}

int GmshCase::outputType() const
{
  int otype=-1;

  std::string ext=outputMeshFile_.extension().string();
  auto i = meshFormats.find(ext);
  if (i!=meshFormats.end())
  {
      otype=i->second;
  }
  else
  {
      insight::Warning("Mesh file extension "+ext+" is unrecognized!");
  }
  return otype;
}




void GmshCase::insertMeshingCommand()
{
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.Algorithm = "+boost::lexical_cast<std::string>(algo2D_), /* 1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=bamg, 8=delquad */
    "Mesh.Algorithm3D = "+boost::lexical_cast<std::string>(algo3D_), /* 1=Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree */
    "Mesh.CharacteristicLengthMin = "+boost::lexical_cast<std::string>(Lmin_),
    "Mesh.CharacteristicLengthMax = "+boost::lexical_cast<std::string>(Lmax_),

    "Mesh.Smoothing = 10",
    "Mesh.SmoothNormals = 1",
    "Mesh.Explode = 1"
  });


  if (outputType()==51) // *.key
  {
      insertLinesBefore(endOfMeshingOptions_, {
          "Mesh.SaveGroupsOfNodes = 1;"
      });
  }


  if (outputType()==27)
  {
    // STL surface mesh
    insertLinesBefore(endOfMeshingActions_, {
      "Mesh 2"
    });
  }
  else
  {
    // volume mesh
    insertLinesBefore(endOfMeshingActions_, {
      "Mesh 3",
      "Coherence Mesh"
    });
  }
}

void GmshCase::doMeshing(int nthread)
{


  std::string ext=outputMeshFile_.extension().string();

  int otype = outputType();
  if (otype==27) // STL
  {
    insertLinesBefore(endOfMeshingOptions_, {
                        "Mesh.Binary=1"
                      });
    setMinimumCirclePoints(20);
  }
  else if (otype==1)
  {
    std::string versionOption;
    switch (mshFileVersion_)
    {
      case v10: versionOption="1.0"; break;
      case v20: versionOption="2.0"; break;
      case v22: versionOption="2.2"; break;
      case v30: versionOption="3.0"; break;
      case v40: versionOption="4.0"; break;
      case v41: versionOption="4.1"; break;
    }
    insertLinesBefore(endOfMeshingOptions_, {
                        "Mesh.MshFileVersion="+versionOption
                      });
  }

  insertLinesBefore(endOfMeshingOptions_, {
                      "Mesh.Format="+boost::lexical_cast<std::string>(otype)
                    });

  insertMeshingCommand();

  // insert write statement
  insertLinesBefore(endOfMeshingActions_, {
    "Save \""+boost::filesystem::absolute(outputMeshFile_).string()+"\""
  });

  // write file
  boost::filesystem::path inputFile = workDir_ / (outputMeshFile_.stem().string() + ".geo");
  {
    std::ofstream f(inputFile.string());
    f << *this;
  }


  if (otype>=0)
  {

    SoftwareEnvironment ee;

    std::vector<std::string> argv;

    argv.insert(argv.end(), {
                  "-v", "10",
                  "-nt", boost::lexical_cast<std::string>(nthread),
                  boost::filesystem::absolute(inputFile).string(),
                  "-"
                });

    auto job = ee.forkCommand( executable_.string(), argv );
    job->runAndTransferOutput();
  }
}




std::ostream& operator<<(std::ostream& os, GmshCase& gc)
{
  for (const auto& l: gc)
  {
    if (!l.empty())
      os << l << ";";
    os << "\n";
  }
  return os;
}



}
} // namespace insight
