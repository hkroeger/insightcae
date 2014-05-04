/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "parser.h"
#include "boost/locale.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/foreach.hpp"
#include "boost/filesystem.hpp"

#include "dxfwriter.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace insight {
namespace cad {
  
namespace parser {  
  
// solidmodel import(const boost::filesystem::path& filepath)
// {
//   cout << "reading model "<<filepath<<endl;
//   return solidmodel(new SolidModel(filepath));
// }

double dot(const vector& v1, const vector& v2)
{
  return arma::as_scalar(arma::dot(v1,v2));
}

FeatureSet queryEdges(const SolidModel& m, const Filter::Ptr& f)
{
  using namespace std;
  using namespace insight::cad;
  return m.query_edges(*f);
}

void writeViews(const boost::filesystem::path& file, const solidmodel& model, const std::vector<viewdef>& viewdefs)
{
  SolidModel::Views views;
  BOOST_FOREACH(const viewdef& vd, viewdefs)
  {
    bool sec=boost::get<3>(vd);
    cout<<"is_section="<<sec<<endl;
    views[boost::get<0>(vd)]=model->createView
    (
      boost::get<1>(vd),
      boost::get<2>(vd),
      sec
    );
  }
  
  {
    DXFWriter::writeViews(file, views);
  }
}

Model::Ptr loadModel(const std::string& name)
{
  std::vector<std::string> paths;
  const char* e=getenv("ISCAD_MODEL_PATH");
  if (e)
  {
    boost::split(paths, e, boost::is_any_of(":"));
  }
  paths.insert(paths.begin(), ".");
  
  BOOST_FOREACH(const std::string& ps, paths)
  {
    path p(ps); 
    p=p/(name+".iscad");
    if (exists(p))
    {
      Model::Ptr model;
      parseISCADModelFile(p, model);
      return model;
    }
  }
  
  return Model::Ptr();
}

}
using namespace parser;


bool parseISCADModelFile(const boost::filesystem::path& fn, parser::Model::Ptr& m)
{
  std::ifstream f(fn.c_str());
  return parseISCADModelStream(f, m);
}

void ModelStepsWriter::operator() (std::string s, SolidModel::Ptr ct)
{
  //std::string s(ws.begin(), ws.end());
  //cout<<s<<endl<<ct<<endl;
  //(*this)[s]=ct;
  if (s=="final")
  {
    ct->saveAs(s+".brep");
    //ct->createView(vec3(0, 0, 0), vec3(0, -1, 0), false);
  }
}
    
bool parseISCADModelStream(std::istream& in, parser::Model::Ptr& m)
{
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  //in.close();
  return parser::parseISCADModel< ISCADParser<std::string::iterator> >(contents_raw.begin(), contents_raw.end(), m);
}

}
}