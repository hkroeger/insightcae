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

#include "emesh.h"
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


eMesh::eMesh()
{}

eMesh::eMesh(const EMeshPtsList &pts)
{
    for (int i=0; i<pts.size(); ++i)
    {
        points_.push_back(pts[i]);
        if (i>0) edges_.push_back({i-1, i});
    }
}

eMesh::eMesh(const EMeshPtsListList &pts)
{

    for (const EMeshPtsList& points: pts)
    {
      for (const arma::mat& p: points)
      {
        points_.push_back(p);
      }
    }

    int ofs=0;
    for (const EMeshPtsList& points: pts)
    {
      for (size_t i=1; i<points.size(); i++)
      {
        edges_.push_back({ofs+i-1, ofs+i});
      }
      ofs+=points.size();
    }
}

eMesh::eMesh(
    const std::vector<arma::mat>&pts,
    const std::vector<std::pair<int, int> >&edges )
  : points_(pts),
    edges_(edges)
{}

int eMesh::nPoints() const
{
    return points_.size();
}

int eMesh::nEdges() const
{
    return edges_.size();
}

void eMesh::write(std::ostream &f) const
{
    f<<"FoamFile {"<<endl
     <<" version     2.0;"<<endl
     <<" format      ascii;"<<endl
     <<" class       featureEdgeMesh;"<<endl
     <<" location    \"\";"<<endl
     <<" object      \"export.eMesh\";"<<endl
     <<"}"<<endl;

    f<<points_.size()<<endl
     <<"("<<endl;
    for (const arma::mat& p: points_)
    {
      f<<OFDictData::vector3(p)<<endl;
    }
    f<<")"<<endl;

    f<<edges_.size()<<endl
     <<"("<<endl;
    for (const auto&e: edges_)
    {
      f<<"("<<e.first<<" "<<e.second<<")"<<endl;
    }
    f<<")"<<endl;
}

void eMesh::write(const boost::filesystem::path& filename) const
{
    std::ofstream f(filename.string());
    write(f);
}

std::ostream &operator<<(std::ostream& os, const eMesh& emesh)
{
    emesh.write(os);
    return os;
}

void exportEMesh(const EMeshPtsList &points, const boost::filesystem::path& filename)
{
    eMesh(points).write(filename);

//  f<<"FoamFile {"<<endl
//   <<" version     2.0;"<<endl
//   <<" format      ascii;"<<endl
//   <<" class       featureEdgeMesh;"<<endl
//   <<" location    \"\";"<<endl
//   <<" object      "<<filename.filename().string()<<";"<<endl
//   <<"}"<<endl;

//  f<<points.size()<<endl
//   <<"("<<endl;
//  for (const arma::mat& p: points)
//  {
//    f<<OFDictData::to_OF(p)<<endl;
//  }
//  f<<")"<<endl;

//  f<<(points.size()-1)<<endl
//   <<"("<<endl;
//  for (size_t i=1; i<points.size(); i++)
//  {
//    f<<"("<<(i-1)<<" "<<i<<")"<<endl;
//  }
//  f<<")"<<endl;
}

void exportEMesh(const std::vector<EMeshPtsList>& pts, const boost::filesystem::path& filename)
{
    eMesh(pts).write(filename);

//  f<<"FoamFile {"<<endl
//   <<" version     2.0;"<<endl
//   <<" format      ascii;"<<endl
//   <<" class       featureEdgeMesh;"<<endl
//   <<" location    \"\";"<<endl
//   <<" object      "<<filename.filename().string()<<";"<<endl
//   <<"}"<<endl;

//  int npts=0;
//  for (const EMeshPtsList& points: pts)
//  {
//    npts+=points.size();
//  }
//  f<<npts<<endl
//   <<"("<<endl;
//  for (const EMeshPtsList& points: pts)
//  {
//    for (const arma::mat& p: points)
//    {
//      f<<OFDictData::to_OF(p)<<endl;
//    }
//  }
//  f<<")"<<endl;

//  f<<(npts-pts.size())<<endl
//   <<"("<<endl;
//  int ofs=0;
//  for (const EMeshPtsList& points: pts)
//  {
//    for (size_t i=1; i<points.size(); i++)
//    {
//      f<<"("<<(ofs+i-1)<<" "<<(ofs+i)<<")"<<endl;
//    }
//    ofs+=points.size();
//  }
//  f<<")"<<endl;
}


}
