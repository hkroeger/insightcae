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

#include "meshing.h"

#include "boost/foreach.hpp"

#include "base/softwareenvironment.h"

#include <iterator>

using namespace std;
using namespace boost;
namespace fs=boost::filesystem;

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




void GmshCase::insertLinesBefore(GmshCase::iterator i, const std::vector<string> &lines)
{
  for (auto j=lines.begin(); j!=lines.end(); ++j)
  {
    insert(i, *j);
  }
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
    "Merge \""+fs::absolute(geomFile).string()+"\""
                    });
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
    "Mesh.MinimumCirclePoints="+lexical_cast<string>(mp)
  });
}

std::set<int> GmshCase::findNamedDefinition(const string &keyword, const string &name) const
{
  std::set<int> result;

  boost::regex re(keyword+" *\\(\"(.*)\"\\) *= *{(.*)}");
  for (const auto& l: *this)
  {
    smatch m;
    if (regex_search(l, m, re))
    {
      if (m[1]==name)
      {
        std::vector<string> nums;
        boost::split(nums, m[2], boost::is_any_of(","));
        std::transform(nums.begin(), nums.end(), std::inserter(result, result.begin()),
                       [](const string& n) { return lexical_cast<int>(n); });
        return result;
      }
    }
  }

  return result;
}

std::set<int> GmshCase::findNamedEdges(const string &name) const
{
  return findNamedDefinition("Physical Line", name);
}

std::set<int> GmshCase::findNamedFaces(const string &name) const
{
  return findNamedDefinition("Physical Surface", name);
}

std::set<int> GmshCase::findNamedSolids(const string &name) const
{
  return findNamedDefinition("Physical Volume", name);
}

void GmshCase::nameVertices(const std::string& name, const FeatureSet& vertices)
{
  std::vector<string> nums;

  std::transform(vertices.data().begin(), vertices.data().end(), std::back_inserter(nums),
                 [](int i) { return lexical_cast<string>(i); });

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Point(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}


void GmshCase::nameEdges(const std::string& name, const FeatureSet& edges)
{
  std::vector<string> nums;

  std::transform(edges.data().begin(), edges.data().end(), std::back_inserter(nums),
                 [](int i) { return lexical_cast<string>(i); });

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Line(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}

void GmshCase::nameFaces(const std::string& name, const FeatureSet& faces)
{
  std::vector<string> nums;

  std::transform(faces.data().begin(), faces.data().end(), std::back_inserter(nums),
                 [](int i) { return lexical_cast<string>(i); });

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Surface(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}

void GmshCase::nameSolids(const std::string& name, const FeatureSet& solids)
{
  std::vector<string> nums;

  std::transform(solids.data().begin(), solids.data().end(), std::back_inserter(nums),
                 [](int i) { return lexical_cast<string>(i); });

  insertLinesBefore(endOfNamedVerticesDefinition_, {
    "Physical Volume(\""+name+"\") = {"+boost::join(nums, ",")+"}"
  });
}

void GmshCase::addSingleNamedVertex(const std::string& vn, const arma::mat& p)
{
  additionalPoints_++;
  int id=part_->allVertices().data().size()+additionalPoints_;
  insertLinesBefore(endOfGeometryDefinition_, {
    str( format("Point(%d) = {%g, %g, %g, 999};")%id%p(0)%p(1)%(p(2)) ),
    str( format("Physical Point(\"%s\") = {%d};")%vn%id )
  });
}




void GmshCase::setVertexLen(const std::string& vn, double L)
{
  insertLinesBefore(endOfGeometryDefinition_, {
    "Characteristic Length{\""+vn+"\"}="+lexical_cast<string>(L)
  });
}




void GmshCase::setEdgeLen(const std::string& en, double L)
{ 
  FeatureSet fs(part_, cad::Edge);
  fs.setData( findNamedEdges(en) );
  FeatureSet vs = part_->verticesOfEdges(fs);
  
  std::vector<string> nums;
  std::transform(vs.data().begin(), vs.data().end(),
                 std::back_inserter(nums),
                 [](int i) { return lexical_cast<string>(i); });

  insertLinesBefore(endOfGeometryDefinition_, {
    "Characteristic Length{"+join(nums, ",")+"}="+lexical_cast<string>(L)
                    });
}




void GmshCase::setFaceEdgeLen(const std::string& fn, double L)
{
  FeatureSet fs(part_, cad::Face);
  fs.setData( findNamedFaces(fn) );
  FeatureSet vs = part_->verticesOfFaces(fs);

  std::vector<string> nums;
  std::transform(vs.data().begin(), vs.data().end(),
                 std::back_inserter(nums),
                 [](int i) { return lexical_cast<string>(i); });

  insertLinesBefore(endOfGeometryDefinition_, {
    "Characteristic Length{"+join(nums, ",")+"}="+lexical_cast<string>(L)
                    });
}

int GmshCase::outputType() const
{
  int otype=-1;

  std::string ext=outputMeshFile_.extension().string();
  if (ext==".stl") otype=27;
  else if (ext==".msh") otype=1;
  else if (ext==".unv") otype=2;
  else if (ext==".med") otype=33;
  else
    insight::Warning("Mesh file extension "+ext+" is unrecognized!");
  return otype;
}




void GmshCase::insertMeshingCommand()
{
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.Algorithm = "+lexical_cast<std::string>(algo2D_), /* 1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=bamg, 8=delquad */
    "Mesh.Algorithm3D = "+lexical_cast<std::string>(algo3D_), /* 1=Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree */
    "Mesh.CharacteristicLengthMin = "+lexical_cast<string>(Lmin_),
    "Mesh.CharacteristicLengthMax = "+lexical_cast<string>(Lmax_),

    "Mesh.Smoothing = 10",
    "Mesh.SmoothNormals = 1",
    "Mesh.Explode = 1"
  });


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

void GmshCase::doMeshing()
{


  std::string ext=outputMeshFile_.extension().string();

  int otype = outputType();
  if (otype==27)
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
                      "Mesh.Format="+lexical_cast<string>(otype)
                    });

  insertMeshingCommand();

  // insert write statement
  insertLinesBefore(endOfMeshingActions_, {
    "Save \""+fs::absolute(outputMeshFile_).string()+"\""
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
                  fs::absolute(inputFile).string(),
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




SurfaceGmshCase::SurfaceGmshCase(
    cad::ConstFeaturePtr part,
    const boost::filesystem::path& outputMeshFile,
    double Lmax, double Lmin,
    const std::string& name,
    bool keepDir,
    bool recombineTris
    )
  : cad::GmshCase(part, outputMeshFile,
                  Lmax, Lmin, keepDir)
{
  if (recombineTris)
  {
    insertLinesBefore(endOfMeshingOptions_, {
      "Mesh.RecombinationAlgorithm = 0",
      "Mesh.RecombineAll = 1",
    });
  }
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.SecondOrderIncomplete=1",
    "Mesh.Optimize = 1",
    "Mesh.OptimizeNetgen = 1",
    "Physical Surface(\""+name+"\")=Surface{:}"
   });
  setQuadratic();
}




SheetExtrusionGmshCase::SheetExtrusionGmshCase(
    cad::ConstFeaturePtr part,
    const std::string& solidName,
    const boost::filesystem::path& outputMeshFile,
    double L, double h, int nLayers,
    const std::vector<NamedEntity>& namedBottomFaces,
    const std::vector<NamedEntity>& namedTopFaces,
    const std::vector<NamedEntity>& namedLateralEdges,
    double grading,
    bool keepDir,
    bool recombineTris
    )
  : cad::GmshCase(part, outputMeshFile,
                  L, L, keepDir),
    grading_(grading)
{
  insight::assertion(grading_>0., "grading must be larger than zero!");

  for (const auto& nbf: namedBottomFaces)
  {
    for (const auto& fi: nbf.second->data())
    {
      namedBottomFaces_[fi]=nbf.first;
    }
  }
  for (const auto& ntf: namedTopFaces)
  {
    for (const auto& fi: ntf.second->data())
    {
      namedTopFaces_[fi]=ntf.first;
    }
  }
  for (const auto& nle: namedLateralEdges)
  {
    for (const auto& ei: nle.second->data())
    {
      namedLateralEdges_[ei]=nle.first;
    }
  }

  if (recombineTris)
  {
    insertLinesBefore(endOfMeshingOptions_, {
      "Mesh.RecombinationAlgorithm = 0",
      "Mesh.RecombineAll = 1",
     });
  }

  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.SecondOrderIncomplete=1",
    "Mesh.Optimize = 1",
    "Physical Volume(\""+solidName+"\") = {}"
   });

  auto insertPhysical = [&](const std::string& entityTypeName, const std::map<cad::FeatureID, std::string>& nfs)
  {
    std::set<std::string> names;

    std::transform(nfs.begin(), nfs.end(),
                   std::inserter(names, names.begin()),
                   [](const std::map<cad::FeatureID, std::string>::value_type& i) { return i.second; } );

    for (const auto& nbf: names)
    {
      insertLinesBefore(endOfMeshingOptions_, {
                          "Physical "+entityTypeName+"(\""+nbf+"\")={}"
                        });
    }
  };

  insertPhysical("Surface", namedBottomFaces_);
  insertPhysical("Surface", namedTopFaces_);
  insertPhysical("Surface", namedLateralEdges_);


  // insert faces one by one
  auto faces=part->allFacesSet();

  std::string layerSpecification;

  if (fabs(1.-grading_)<1e-10)
  {
    layerSpecification =
      str( format("Layers{%d}") % nLayers );
  }
  else
  {
    double g=pow(grading_, 1./double(nLayers-1));

    arma::mat h = arma::ones(nLayers);
    for (int i=1; i<nLayers; ++i)
      h(i)=g*h(i-1);
    h/=arma::as_scalar(sum(h));

    arma::mat hcum = arma::zeros(nLayers);
    hcum(0)=h(0);
    for (int i=1; i<nLayers; ++i)
    {
      hcum(i)=hcum(i-1)+h(i);
    }

    std::string layerNum,layerHeight;
    for (int i=0;i<nLayers;++i)
    {
      layerNum += "1";
      layerHeight += boost::lexical_cast<std::string>( hcum(i) );
      if (i<nLayers-1)
      {
        layerNum+=",";
        layerHeight+=",";
      }
    }
    layerSpecification="Layers{ {"+layerNum+"}, {"+layerHeight+"} }";
  }

  for (FeatureID fi : faces)
  {
    std::string out=str(format("out%d")%fi);

    insertLinesBefore(endOfMeshingActions_, {
      str(format(out+"[] = Extrude {0.,0.,%g} { Surface{%d}; %s; Recombine; }")
                        % h % fi % layerSpecification ),
      "Physical Volume(\""+solidName+"\") += "+out+"[1]"
    });
  }


  for (FeatureID fi : faces)
  {
    std::string out=str(format("out%d")%fi);

    // get list of the edge IDs, sort by their ID
    std::vector<FeatureID> currentFaceEdges;
    for (TopExp_Explorer ex(part->face(fi), TopAbs_EDGE); ex.More(); ex.Next())
    {
      currentFaceEdges.push_back(part->edgeID(ex.Current()));
    }
    std::sort(currentFaceEdges.begin(), currentFaceEdges.end());

    auto nbf=namedBottomFaces_.find(fi);
    if (nbf!=namedBottomFaces_.end())
    {
      insertLinesBefore(endOfMeshingActions_, {
        str(format("Physical Surface(\"%s\") += {%d}") % nbf->second % nbf->first)
                        });
    }
    auto ntf=namedTopFaces_.find(fi);
    if (ntf!=namedTopFaces_.end())
    {
      insertLinesBefore(endOfMeshingActions_, {
        str(format("Physical Surface(\"%s\") += %s[0]") % ntf->second % out)
                        });
    }

    for (size_t i=0; i<currentFaceEdges.size(); i++)
    {
      auto eid = currentFaceEdges[i];
      auto nle = namedLateralEdges_.find(eid);
      if (nle!=namedLateralEdges_.end())
      {
        insertLinesBefore(endOfMeshingActions_, {
          str(format("Physical Surface(\"%s\") += %s[%d]")
                            % nle->second % out % (2+i) )
                          });
      }
    }
  }

}


}
}
