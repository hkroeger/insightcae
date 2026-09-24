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

#ifndef INSIGHT_OFPATCHUTILS_H
#define INSIGHT_OFPATCHUTILS_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "base/linearalgebra.h"

namespace insight
{

class OpenFOAMCase;
class OFEnvironment;

void setSet(const OpenFOAMCase& ofc, const boost::filesystem::path& location, const std::vector<std::string>& cmds);

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap=true);

/**
 * Converts a pair of patches into a cyclic pair using createPatch.
 * The names of the two patches must be of the pattern (.*)_half[0,1].
 * Only the name prefix (in parantheses_) must be supplied as an argument.
 */
void convertPatchPairToCyclic
(
  const OpenFOAMCase& ofc,
  const boost::filesystem::path& location,
  const std::string& namePrefix
);


class patchIntegrate
{
public:
  patchIntegrate(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& location,
        const std::string& fieldName,
        const std::string& patchNamePattern,
        const std::string& regionName = std::string(),
        const std::vector<std::string>& addopts = {"-latestTime"}
        );

  /**
   * @brief t_
   * time/iteration values for subsequent arrays
   */
  arma::mat t_;

  /**
   * @brief A_vs_t_
   * area for different times/iterations
   */
  arma::mat A_;

  /**
   * @brief int_vs_t_
   * integral values for different times/iterations
   */
  arma::mat integral_values_;

  size_t n() const;
};


class patchArea
{
public:
  patchArea(const OpenFOAMCase& cm, const boost::filesystem::path& location,
            const std::string& patchName);

  double A_;
  arma::mat n_;
  arma::mat ctr_;
};


arma::mat projectedArea
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location,
  const arma::mat& direction,
  const std::vector<std::string>& patches,
    const std::vector<std::string>& addopts = {"-latestTime"}
);

arma::mat minPatchPressure
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location,
  const std::string& patch,
  const double& Af=0.025,
    const std::vector<std::string>& addopts = {"-latestTime"}
);

void createBaffles
(
  const OpenFOAMCase& c,
  const boost::filesystem::path& location,
  const std::string& faceZoneName
);

/**
 * return extrema in specified zone
 * @return pair of min and max, first col time, others one col per component
 */
std::pair<arma::mat, arma::mat> zoneExtrema
(
  const OpenFOAMCase& cm,
  const boost::filesystem::path& location,
  const std::string fieldName,
  const std::string zoneName,
    const std::vector<std::string>& addopts = {"-latestTime"}
);

void removeCellSetFromMesh
(
  const OpenFOAMCase& c,
  const boost::filesystem::path& location,
  const std::string& cellSetName
);


std::set<std::string> readPatchNameList(
        const OpenFOAMCase& cm,
        const boost::filesystem::path& caseLocation,
        bool parallel,
        const std::string& regionName = std::string(),
        const std::string& time = "constant");

class PatchLayers
        : public std::map<std::string, int>
{
public:
    PatchLayers();
    PatchLayers(
            const OpenFOAMCase& cm,
            const boost::filesystem::path& caseLocation,
            bool parallel,
            const std::string& regionName = std::string(),
            const std::string& time = "constant" );

    void setByPattern(const std::string& regex_pattern, int nLayers);
};


arma::mat surfaceProjectLine
(
  const OFEnvironment& ofe,
  const boost::filesystem::path& surfaceFile,
  const arma::mat& start, const arma::mat& end, int npts,
  const arma::mat& projdir
);


std::vector<std::string> patchList
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseDir,
    const std::string& include=".*",
    const std::vector<std::string>& exclude = std::vector<std::string>() // OF syntax: either string or regex (in quotes)
);

}

#endif // INSIGHT_OFPATCHUTILS_H
