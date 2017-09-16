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

namespace insight {
namespace cad {

GmshCase::GmshCase(const insight::cad::Feature& part, double Lmax, double Lmin)
: part_(part),
  Lmax_(Lmax),
  Lmin_(Lmin),
  elementOrder_(2),
  secondOrderLinear_(0),
  additionalPoints_(0)
{
}

void GmshCase::nameVertices(const std::string& name, const FeatureSet& vertices)
{
  NamedFeatureSet::iterator i = namedVertices_.find(name);
  if (i!=namedVertices_.end())
  {
    i->second->safe_union(vertices);
  }
  else 
  {
    namedVertices_.insert(NamedFeatureSet::value_type(name, vertices.clone()));
  }
}


void GmshCase::nameEdges(const std::string& name, const FeatureSet& edges)
{
  NamedFeatureSet::iterator i=namedEdges_.find(name);
  if (i!=namedEdges_.end())
  {
    i->second->safe_union(edges);
  }
  else 
  {
    namedEdges_.insert(NamedFeatureSet::value_type(name, edges.clone())); // .insert(edges.begin(), edges.end());
  }
}

void GmshCase::nameFaces(const std::string& name, const FeatureSet& faces)
{
  NamedFeatureSet::iterator i=namedFaces_.find(name);
  if (i!=namedFaces_.end())
  {
    i->second->safe_union(faces);
  }
  else 
  {
    namedFaces_.insert(NamedFeatureSet::value_type(name, faces.clone()));
  }
}

void GmshCase::addSingleNamedVertex(const std::string& vn, const arma::mat& p)
{
  additionalPoints_++;
  int id=part_.allVertices().data().size()+additionalPoints_;
  std::ostringstream oss;
  oss<<"Point("<< id <<") = {"<<p(0)<<", "<<p(1)<<", "<<p(2)<<", 999};\n";
  oss<<"Physical Point(\""<< vn <<"\") = {"<<id<<"};\n";
  options_.push_back(oss.str());  
}


void GmshCase::setVertexLen(const std::string& vn, double L)
{
  std::ostringstream oss;
  oss<<"Characteristic Length{";
  
  const FeatureSetData& fs = (namedVertices_.find(vn)->second)->data();
  
  for (FeatureSetData::const_iterator i=fs.begin(); i!=fs.end(); i++)
  {
    if (i!=fs.begin()) oss<<",";
    oss<<*i;
  }
  oss<<"}="<<L<<";";
  options_.push_back(oss.str());

}


void GmshCase::setEdgeLen(const std::string& en, double L)
{
  std::ostringstream oss;
  oss<<"Characteristic Length{";
  
  const FeatureSet& fs = *(namedEdges_.find(en)->second);
  FeatureSet vs = part_.verticesOfEdges(fs);
  
  for (FeatureSetData::const_iterator i=vs.data().begin(); i!=vs.data().end(); i++)
  {
    if (i!=vs.data().begin()) oss<<",";
    oss<<*i;
  }
  oss<<"}="<<L<<";";
  options_.push_back(oss.str());
}

void GmshCase::setFaceEdgeLen(const std::string& fn, double L)
{
  std::ostringstream oss;
  oss<<"Characteristic Length{";
  
  const FeatureSet& fs = *(namedFaces_.find(fn)->second);
  FeatureSet vs = part_.verticesOfFaces(fs);
  
  for (FeatureSetData::const_iterator i=vs.data().begin(); i!=vs.data().end(); i++)
  {
    if (i!=vs.data().begin()) oss<<",";
    oss<<*i;
  }
  oss<<"}="<<L<<";";
  options_.push_back(oss.str());
}

void GmshCase::doMeshing
(
  const std::string& vname,
  const boost::filesystem::path& outputMeshFile,
  bool keeptmpdir
)
{
//   boost::filesystem::path inputFile = boost::filesystem::unique_path("%%%%-%%%%-%%%%.geo");
  boost::filesystem::path tmpWorkDir = boost::filesystem::unique_path();

  int otype=-1;
  std::string ext=outputMeshFile.extension().string();
  if (ext==".msh") otype=1;
  else if (ext==".unv") otype=2;
  else if (ext==".med") otype=33;
  else if (ext==".stl") otype=27;
  else if (ext==".geo") 
  {
    otype=-1;
    tmpWorkDir = outputMeshFile.stem().string() + "_gmsh";
    keeptmpdir=true;
  }
  else throw insight::Exception("Mesh file extension "+ext+" is unrecognized!");
  
  create_directories(tmpWorkDir);
  boost::filesystem::path inputFile = tmpWorkDir / (outputMeshFile.stem().string() + ".geo");
  boost::filesystem::path geomFile = tmpWorkDir / (outputMeshFile.stem().string() + ".brep");
  
  std::ofstream f(inputFile.c_str());
  
  f<<
  "Geometry.AutoCoherence = 0;\n"
  "Geometry.OCCSewFaces = 0;\n"
  "Geometry.Tolerance = 1e-10;\n"
  //"Mesh.LcIntegrationPrecision = 1e-12;\n"
  //"Mesh.HighOrderOptimize = 5;\n"
  ;
  
  part_.saveAs(geomFile.string());
  
  f<<"Merge \""<< absolute(geomFile).string() <<"\";\n";

  BOOST_FOREACH(const std::string& o, options_)
  {
    f<<o<<endl;
  }

  BOOST_FOREACH(const NamedFeatureSet::value_type& ne, namedVertices_)
  {
    f<<"Physical Point(\""<< ne.first <<"\") = {";
    for (FeatureSetData::const_iterator j=ne.second->data().begin(); j!=ne.second->data().end(); j++)
    {
      if (j!=ne.second->data().begin()) f<<",";
      f<<*j;
    }
    f<<"};\n";
  }
  
  BOOST_FOREACH(const NamedFeatureSet::value_type& ne, namedEdges_)
  {
    f<<"Physical Line(\""<< ne.first <<"\") = {";
    for (FeatureSetData::const_iterator j=ne.second->data().begin(); j!=ne.second->data().end(); j++)
    {
      if (j!=ne.second->data().begin()) f<<",";
      f<<*j;
    }
    f<<"};\n";
  }
  
  BOOST_FOREACH(const NamedFeatureSet::value_type& nf, namedFaces_)
  {
    f<<"Physical Surface(\""<< nf.first <<"\") = {";
    for (FeatureSetData::const_iterator j=nf.second->data().begin(); j!=nf.second->data().end(); j++)
    {
      if (j!=nf.second->data().begin()) f<<",";
      f<<*j;
    }
    f<<"};\n";
  }

  f<<"Physical Volume(\"" << vname << "\") = {1};\n";

  f<<
  "Mesh.Algorithm = 1; /* 1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=bamg, 8=delquad */\n"
  "Mesh.Algorithm3D = 4; /* 1=Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree */\n"
  "Mesh.CharacteristicLengthMin = "<<Lmin_<<";\n"
  "Mesh.CharacteristicLengthMax = "<<Lmax_<<";\n"
  "Mesh.ElementOrder="<<elementOrder_<<";\n"
  "Mesh.SecondOrderLinear="<<secondOrderLinear_<<";\n"
  "Mesh.Smoothing = 10;\n"
  "Mesh.SmoothNormals = 1;\n"
  "Mesh.Explode = 1;\n";
  if (otype==27)
  {
      f<<"Mesh.CharacteristicLengthFromCurvature=1;\n";
  }
  
  if (otype>=0)
  {
    f<<
    "Mesh.Format="<< otype <<"; /* 1=msh, 2=unv, 10=automatic, 19=vrml, 27=stl, 30=mesh, 31=bdf, 32=cgns, 33=med, 40=ply2 */\n";
    if (otype==27)
    {
        f<<"Mesh.Binary=1;\n";
    }
  }

  f.close();

  if (otype>=0)
  {
      
    std::string cmd="gmsh";
    if (otype==27)
        cmd+=" -2 -v 2 ";
    else
        cmd+=" -3 -v 2 ";
    cmd+=boost::filesystem::absolute(inputFile).string()+" -o "+boost::filesystem::absolute(outputMeshFile).string();
    
    int r=system(cmd.c_str());
    if (r)
      throw insight::Exception("Execution of gmsh failed!");
  }
  
//   boost::filesystem::remove(inputFile);
//   boost::filesystem::remove(geomFile);

  if (!keeptmpdir)
    boost::filesystem::remove_all(tmpWorkDir);
  
}



}
}
