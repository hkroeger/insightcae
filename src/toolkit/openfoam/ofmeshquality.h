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

#ifndef INSIGHT_OFMESHQUALITY_H
#define INSIGHT_OFMESHQUALITY_H

#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "base/linearalgebra.h"
#include "base/resultset.h"

namespace insight
{

class OpenFOAMCase;

struct MeshQualityInfo
{
  std::string time;

  int ncells;
  int nhex, nprism, ntet, npoly;

  int nmeshregions;

  arma::mat bb_min, bb_max;
  double max_aspect_ratio;
  std::string min_faceA, min_cellV;

  double max_nonorth, avg_nonorth;
  int n_severe_nonorth;

  int n_neg_facepyr;

  double max_skewness;
  int n_severe_skew;

  MeshQualityInfo();
};

typedef std::vector<MeshQualityInfo> MeshQualityList;

MeshQualityList getMeshQuality(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    const std::vector<std::string>& addopts
    );

void meshQualityReport(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& location,
    ResultSet& results,
    const std::vector<std::string>& addopts = {"-latestTime"}
);

}

#endif // INSIGHT_OFMESHQUALITY_H
