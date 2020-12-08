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
    const std::string& exeName,
    bool keepDir
    )
: workDir_(keepDir),
  part_(part),
  additionalPoints_(0),
  executableName_(exeName),
  outputMeshFile_(outputMeshFile)
{
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

  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.Algorithm = 1", /* 1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=bamg, 8=delquad */
    "Mesh.Algorithm3D = 4", /* 1=Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree */
    "Mesh.CharacteristicLengthMin = "+lexical_cast<string>(Lmin),
    "Mesh.CharacteristicLengthMax = "+lexical_cast<string>(Lmax),

    "Mesh.Smoothing = 10",
    "Mesh.SmoothNormals = 1",
    "Mesh.Explode = 1"
   });
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




void GmshCase::doMeshing()
{


  std::string ext=outputMeshFile_.extension().string();

  int otype=-1;
  if (ext==".stl")
  {
    otype=27;
    insertLinesBefore(endOfMeshingOptions_, {
                        "Mesh.Binary=1"
                      });
    setMinimumCirclePoints(20);
  }
  else if (ext==".msh") otype=1;
  else if (ext==".unv") otype=2;
  else if (ext==".med") otype=33;
  else
    insight::Warning("Mesh file extension "+ext+" is unrecognized!");

  insertLinesBefore(endOfMeshingOptions_, {
                      "Mesh.Format="+lexical_cast<string>(otype)
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

    if (otype==27)
        argv.push_back("-2");
    else
        argv.push_back("-3");

    argv.insert(argv.end(), {
                  "-v", "10",
                  fs::absolute(inputFile).string(),
                  "-o", fs::absolute(outputMeshFile_).string()
                });

    auto job = ee.forkCommand( executableName_, argv );
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
    bool keepDir
    )
  : cad::GmshCase(part, outputMeshFile,
                  Lmax, Lmin, "gmshinsightcae", keepDir)
{
  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.RecombinationAlgorithm = 0",
    "Mesh.SecondOrderIncomplete=1",
    "Mesh.RecombineAll = 1",
    "Mesh.Optimize = 1",
    "Mesh.OptimizeNetgen = 1",
    "Physical Surface(\""+name+"\")=Surface{:}"
   });
  setQuadratic();
}




SheetExtrusionGmshCase::SheetExtrusionGmshCase(
    cad::ConstFeaturePtr part,
    const boost::filesystem::path& outputMeshFile,
    double L, double h, int nLayers,
    const std::vector<NamedEntity>& namedBottomFaces,
    const std::vector<NamedEntity>& namedTopFaces,
    const std::vector<NamedEntity>& namedLateralEdges,
    bool keepDir
    )
  : cad::GmshCase(part, outputMeshFile,
                  L, L, "gmshinsightcae", keepDir)
{

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


  insertLinesBefore(endOfMeshingOptions_, {
    "Mesh.RecombinationAlgorithm = 0",
    "Mesh.SecondOrderIncomplete=1",
    "Mesh.RecombineAll = 1",
    "Mesh.Optimize = 1",

    "Physical Volume(1) = {}"
   });

//  for (const auto& nbf: namedBottomFaces_)
//  {
//    insertLinesBefore(endOfMeshingOptions_, {
//                        "Physical Surface(\""+nbf.second+"\")={}"
//                      });
//  }
  for (const auto& ntf: namedTopFaces_)
  {
    insertLinesBefore(endOfMeshingOptions_, {
                        "Physical Surface(\""+ntf.second+"\")={}"
                      });
  }
  for (const auto& nle: namedLateralEdges_)
  {
    insertLinesBefore(endOfMeshingOptions_, {
                        "Physical Surface(\""+nle.second+"\")={}"
                      });
  }
  // insert faces one by one
  auto faces=part->allFacesSet();

//  for (FeatureID fi : faces)
//  {
//    auto nbf=namedBottomFaces_.find(fi);
//    if (nbf!=namedBottomFaces_.end())
//    {
//      insertLinesBefore(endOfMeshingActions_, {
//        str(format("Physical Surface(\"%s\") += Surface{%d}")%nbf->second%fi)
//                        });
//    }
//  }


  for (FeatureID fi : faces)
  {
    std::string out=str(format("out%d")%fi);

    insertLinesBefore(endOfMeshingActions_, {
      str(format(out+"[] = Extrude {0.,0.,%g} { Surface{%d}; Layers{%d}; Recombine; }")
                        % h % fi % nLayers ),
      "Physical Volume(1) += "+out+"[1]"
    });
  }

  insertLinesBefore(endOfMeshingActions_, {
    "Coherence Mesh"
  });


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


    auto ntf=namedTopFaces_.find(fi);
    if (ntf!=namedTopFaces_.end())
    {
      insertLinesBefore(endOfMeshingActions_, {
        str(format("Physical Surface(\"%s\") += %s[0]")%ntf->second%out)
                        });
    }

    for (int i=0; i<currentFaceEdges.size(); i++)
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


  setLinear();
}


}
}
