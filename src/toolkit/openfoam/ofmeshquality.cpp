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

#include "ofmeshquality.h"
#include "base/analysis.h"
#include "base/cppextensions.h"
#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/progressdisplayer/textprogressdisplayer.h"
#include "base/tools.h"
#include "base/translations.h"
#include "base/resultelements/attributeresulttable.h"

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


MeshQualityInfo::MeshQualityInfo()
{
  ncells=-1;
  nhex=-1;
  nprism=-1;
  ntet=-1;
  npoly=-1;
  nmeshregions=-1;
  bb_min=vec3(-DBL_MAX, -DBL_MAX, -DBL_MAX);
  bb_max=vec3(DBL_MAX, DBL_MAX, DBL_MAX);
  max_aspect_ratio=-1;
  min_faceA="";
  min_cellV="";
  max_nonorth=-1;
  avg_nonorth=-1;
  max_skewness=-1;
  n_severe_nonorth=0;
  n_neg_facepyr=0;
  n_severe_skew=0;
}

std::vector<MeshQualityInfo> getMeshQuality(const OpenFOAMCase& cm, const boost::filesystem::path& location,
                       const std::vector<string>& addopts
                      )
{
  std::vector<std::string> opts;
  copy(addopts.begin(), addopts.end(), back_inserter(opts));

  std::vector<std::string> output;
  cm.executeCommand(location, "checkMesh", opts, &output);

  // Pattern
  enum Section {MeshStats, CellTypes, Topology, Geometry} ;
  boost::regex SectionIntroPattern[] = {
    boost::regex("^Mesh stats$"),
    boost::regex("^Overall number of cells of each type:$"),
    boost::regex("^Checking topology...$"),
    boost::regex("^Checking geometry...$")
  };
  Section curSection;

  boost::regex re_time("^ *Time = (.+)$");
  boost::match_results<std::string::const_iterator> what;
  std::string time="";



  typedef std::vector<MeshQualityInfo> MQInfoList;
  MQInfoList mqinfos;
  MeshQualityInfo curmq;
  for (const std::string& line: output)
  {
    if (boost::regex_match(line, what, re_time))
    {
      if (curmq.time!="")
      {
        mqinfos.push_back(curmq);
      }
      curmq.time=what[1];
    }
    for (int i=0; i<4; i++)
      if (boost::regex_match(line, what, SectionIntroPattern[i])) curSection=static_cast<Section>(i);

//     try{
    switch (curSection)
    {
      case MeshStats:
      {
        if (boost::regex_match(line, what, boost::regex("^ *cells: *([0-9]+)$")))
          curmq.ncells=lexical_cast<int>(what[1]);
        break;
      }
      case CellTypes:
      {
        if (boost::regex_match(line, what, boost::regex("^ *hexahedra: *([0-9]+)$")))
          curmq.nhex=lexical_cast<int>(what[1]);
        if (boost::regex_match(line, what, boost::regex("^ *prisms: *([0-9]+)$")))
          curmq.nprism=lexical_cast<int>(what[1]);
        if (boost::regex_match(line, what, boost::regex("^ *tetrahedra: *([0-9]+)$")))
          curmq.ntet=lexical_cast<int>(what[1]);
        if (boost::regex_match(line, what, boost::regex("^ *polyhedra: *([0-9]+)$")))
          curmq.npoly=lexical_cast<int>(what[1]);
        break;
      }
      case Topology:
      {
        if (boost::regex_match(line, what, boost::regex("^ *Number of regions: *([^ ]+) .*$")))
          curmq.nmeshregions=lexical_cast<int>(what[1]);
        break;
      }
      case Geometry:
      {
        if (boost::regex_match(line, what, boost::regex("^ *Overall domain bounding box \\(([^ ]+) ([^ ]+) ([^ ]+)\\) \\(([^ ]+) ([^ ]+) ([^ ]+)\\)$")))
        {
          curmq.bb_min=vec3( toNumber<double>(what[1]), toNumber<double>(what[2]), toNumber<double>(what[3]) );
          curmq.bb_max=vec3( toNumber<double>(what[4]), toNumber<double>(what[5]), toNumber<double>(what[6]) );
        }
        if (boost::regex_match(line, what, boost::regex("^ *Max aspect ratio = ([^ ]+) .*$")))
        {
          curmq.max_aspect_ratio=toNumber<double>(what[1]);
        }
        if (boost::regex_match(line, what, boost::regex("^ *Minimum face area = *([^ ]+)\\. Maximum face area = *([^ ]+)\\..*$")))
        {
          cout<<what[1]<<endl;
          curmq.min_faceA=std::string(what[1]); // is a very small value, keep as string
          cout<<curmq.min_faceA<<endl;
          //sscanf(string(what[1]).data(), "%g", &curmq.min_faceA);
        }
        if (boost::regex_match(line, what, boost::regex("^ *Min volume = *([^ ]+)\\. Max volume.*$")))
        {
          cout<<what[1]<<endl;
          curmq.min_cellV=std::string(what[1]); // is a very small value, keep as string
          cout<<curmq.min_cellV<<endl;
          //sscanf(string(what[1]).data(), "%g", &curmq.min_cellV);
        }
        if (boost::regex_match(line, what, boost::regex("^ *Mesh non-orthogonality Max: ([^ ]+) average: ([^ ]+)$")))
        {
          curmq.max_nonorth=toNumber<double>(what[1]);
          curmq.avg_nonorth=toNumber<double>(what[2]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Number of severely non-orthogonal \\(> ([^ ]+) degrees\\) faces: ([^ ]+)\\..*$")))
        {
          curmq.n_severe_nonorth=toNumber<double>(what[2]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Max skewness = ([^ ]+), ([^ ]+) highly skew faces.*$")))
        {
          curmq.n_severe_skew=toNumber<double>(what[2]);
          curmq.max_skewness=toNumber<double>(what[1]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Max skewness = ([^ ]+) OK.*$")))
        {
          curmq.n_severe_nonorth=toNumber<double>(what[1]);
        }
        if (boost::regex_match(line, what, boost::regex("^.*Error in face pyramids: ([^ ]+) faces are incorrectly oriented.*$")))
        {
          curmq.n_neg_facepyr=toNumber<double>(what[1]);
        }
        break;
      }
    }
//     }
//     catch(boost::bad_lexical_cast& e) {
//      cout<<"Lexical_cast: "<<e.what()<<endl;
//     }
  }
  if (curmq.time!="") mqinfos.push_back(curmq);

  return mqinfos;
}

void meshQualityReport(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    ResultSet& results,
    const std::vector<string>& addopts
    )
{
  auto mqinfos=getMeshQuality(cm, location, addopts);
  
  for (const MeshQualityInfo& mq: mqinfos)
  {
    results.insert
    (
     "Mesh quality at time "+mq.time,
     std::make_unique<AttributeTableResult>
     (
        AttributeTableResult::AttributeNames{
          {"Number of cells"},
          {"thereof hexahedra"},
          {"prisms"},
          {"tetrahedra"},
          {"polyhedra"},

          {"Number of mesh regions"},

          {"Domain extent (X)"},
          {"Domain extent (Y)"},
          {"Domain extent (Z)"},

          {"Max. aspect ratio"},
          {"Min. face area"},
          {"Min. cell volume"},

          {"Max. non-orthogonality"},
          {"Avg. non-orthogonality"},
          {"Max. skewness"},

          {"No. of severely non-orthogonal faces"},
          {"No. of negative face pyramids"},
          {"No. of severely skew faces"}
        },

        AttributeTableResult::AttributeValues{
          mq.ncells, mq.nhex, mq.nprism, mq.ntet, mq.npoly,
          mq.nmeshregions,
          mq.bb_max(0)-mq.bb_min(0), mq.bb_max(1)-mq.bb_min(1), mq.bb_max(2)-mq.bb_min(2),
          mq.max_aspect_ratio, mq.min_faceA, mq.min_cellV,
          mq.max_nonorth, mq.avg_nonorth, mq.max_skewness,
          mq.n_severe_nonorth, mq.n_neg_facepyr, mq.n_severe_skew
        },

        "Mesh Quality", "", ""
     )
    ).setOrder(0);
  }
}


}
