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

#include "ofmeshcopy.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"

#include "base/units.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"
#include "openfoam/solveroutputanalyzer.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/createpatch.h"

#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <map>
#include <cmath>
#include <limits>

#include "vtkSTLReader.h"
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkCellData.h"
#include "base/warningdispatcher.h"

using namespace std;
using namespace arma;
using namespace boost;
using namespace boost::filesystem;

namespace insight
{


void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to, bool purify, bool ignoremissing, bool include_zones)
{
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.string();
  ::system(cmd.c_str());
  
  std::vector<std::string> files={"boundary", "faces", "neighbour", "owner", "points"};
  if (include_zones)
  {
    files.push_back("pointZones");
    files.push_back("faceZones");
    files.push_back("cellZones");
  }
  if (purify)
  {
    for (const std::string& fname: files)
    {
      path gzname(fname.c_str()); gzname=(gzname.string()+".gz");
      if (exists(source/gzname)) 
      {
        cout<<"Copying file "<<gzname<<endl;
        if (exists(target/gzname)) remove(target/gzname);
        copy_file(source/gzname, target/gzname);
      }
      else if (exists(source/fname))
      {
        cout<<"Copying file "<<fname<<endl;
        if (exists(target/fname)) remove(target/fname);
        copy_file(source/fname, target/fname);
      }
      else 
        if (!ignoremissing) throw insight::Exception("Essential mesh file "+fname+" not present in "+source.string());
    }
  }
  else
    throw insight::Exception("Not implemented!");
}

void create_symlink_force_overwrite(const path& source, const path& targ)
{
  if (is_symlink(targ))
    remove(targ);
  else
  {
    if (exists(targ))
      throw insight::Exception("Link target "+targ.string()+" exists and is not a symlink! Please remove manually first.");
  }
    
  create_symlink(source, targ);
}

void linkPolyMesh(
    const boost::filesystem::path& from,
    const boost::filesystem::path& to,
    const OFEnvironment* env)
{
  if (env)
  {
    boost::filesystem::path casedir=to.parent_path();
    std::cout<<"Creating case skeleton in "<<casedir<<endl;
    OpenFOAMCase cm(*env);
    cm.insert(new MeshingNumerics(cm));
    cm.createOnDisk(casedir);
  }
  
  path source(from/"polyMesh");
  path target(to/"polyMesh");
  if (!exists(target))
    create_directories(target);
  
  std::string cmd("ls "); cmd+=source.string();
  ::system(cmd.c_str());
  
  {
    std::string fname="boundary";

    // reset all patch types
    // (GGI patches require zones, which need to be recreated.
    // During zone creation, GGI types must not be defined...)
    // => read boundary file, reset types, write to target dir
    OFDictData::dict org_bnd;

    path gzname(fname.c_str()); gzname=(gzname.string()+".gz");
    if (exists(source/gzname)) 
    {
      cout<<"Processing file "<<gzname<<endl;
//      if (exists(target/gzname)) remove(target/gzname);
//      copy_file(source/gzname, target/gzname);
      std::ifstream compressedDict( (source/gzname).string() );
      boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
      in.push(boost::iostreams::gzip_decompressor());
      in.push(compressedDict);
      std::istream bf(&in);
      readOpenFOAMBoundaryDict(bf, org_bnd);
    }
    else if (exists(source/fname))
    {
      cout<<"Processing file "<<fname<<endl;
//      if (exists(target/fname)) remove(target/fname);
//      copy_file(source/fname, target/fname);
      std::ifstream bf( (source/fname).c_str() );
      readOpenFOAMBoundaryDict(bf, org_bnd);
    }
    else 
      throw insight::Exception("Essential mesh file "+fname+" not present in "+source.string());

    OFDictData::dictFile new_bnd;
    for (const auto& b: org_bnd)
    {
      const auto& pn = b.first;
      const auto& pd = b.second;

      OFDictData::dict nbd;
      nbd["type"]="patch";
      for (const auto& copy_key: std::vector<std::string>({"startFace", "nFaces"}))
      {
        nbd[copy_key]=boost::get<const int&>(
              boost::get<const OFDictData::dict&>(pd).at(copy_key)
              );
      }
      new_bnd[pn]=nbd;
    }

    {
      std::ofstream bf( (target/fname).c_str() );
      writeOpenFOAMBoundaryDict(bf, new_bnd);
    }

  }

  for (const std::string& fname:
       {/*"boundary", */"faces", "neighbour", "owner", "points"})
  {
    path gzname(fname.c_str()); gzname=(gzname.string()+".gz");
    if (exists(source/gzname)) create_symlink_force_overwrite(source/gzname, target/gzname);
    else if (exists(source/fname)) create_symlink_force_overwrite(source/fname, target/fname);
    else throw insight::Exception("Essential mesh file "+fname+" not present in "+source.string());
  }
}

void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to)
{
  if (!exists(to))
    create_directories(to);

  directory_iterator end_itr; // default construction yields past-the-end
  for ( directory_iterator itr( from );
	itr != end_itr;
	++itr )
  {
    if ( is_regular_file(itr->status()) )
    {
      copy_file(itr->path(), to/itr->path().filename());
    }
  }
}


}
