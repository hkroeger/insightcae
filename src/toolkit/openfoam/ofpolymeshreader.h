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

#ifndef INSIGHT_OFPOLYMESHREADER_H
#define INSIGHT_OFPOLYMESHREADER_H

#include <cstdint>
#include <map>
#include <utility>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "base/linearalgebra.h"

#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

namespace insight
{

/*
 * Lightweight direct reader for the boundary of an OpenFOAM polyMesh.
 * Parses constant/polyMesh/{boundary,faces,points} (ascii or binary,
 * optionally gzipped, faceList or faceCompactList) without OpenFOAM
 * being installed or linked.
 */


struct OFPolyMeshPatchInfo
{
  std::string type;
  int64_t startFace = 0;
  int64_t nFaces = 0;
};


/**
 * @brief The OFPatchSurface struct
 * Compact surface of one or more boundary patches.
 * Contains only the points, which are referenced by the patch faces.
 */
struct OFPatchSurface
{
  /**
   * @brief points
   * point coordinates, xyz interleaved (size 3*nPoints)
   */
  std::vector<double> points;

  /**
   * @brief faceOffsets
   * start of face i in faceVerts is faceOffsets[i], end is faceOffsets[i+1] (size nFaces+1)
   */
  std::vector<int64_t> faceOffsets;

  /**
   * @brief faceVerts
   * vertex indices into points
   */
  std::vector<int64_t> faceVerts;

  inline size_t nPoints() const { return points.size()/3; }
  inline size_t nFaces() const { return faceOffsets.empty() ? 0 : faceOffsets.size()-1; }
};


/**
 * @return <caseDir>/<time>[/<regionName>]/polyMesh
 */
boost::filesystem::path polyMeshDirectory(
    const boost::filesystem::path& caseDir,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );


std::map<std::string, OFPolyMeshPatchInfo> readPolyMeshBoundary(
    const boost::filesystem::path& polyMeshDir );


/**
 * @brief readPolyMeshPatchSurface
 * read the faces of all patches, whose names match any of the given regular expressions
 * @param polyMeshDir
 * path to the polyMesh directory
 */
OFPatchSurface readPolyMeshPatchSurface(
    const boost::filesystem::path& polyMeshDir,
    const std::vector<std::string>& patchNamePatterns );

OFPatchSurface readPatchSurface(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );


/**
 * @brief toVTKPolyData
 * convert into VTK surface. Faces are polygons (not triangulated).
 */
vtkSmartPointer<vtkPolyData> toVTKPolyData(const OFPatchSurface& surf);

vtkSmartPointer<vtkPolyData> readPatchAsVTKPolyData(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );


/**
 * @brief maxRadius
 * maximum distance of the surface points from an axis
 * @param p0
 * point on axis
 * @param axis
 * axis direction (needs not to be normalized)
 */
double maxRadius(
    const OFPatchSurface& surf,
    const arma::mat& p0,
    const arma::mat& axis );

double maxPatchRadius(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const arma::mat& p0,
    const arma::mat& axis,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );



/**
 * @brief planeDistanceRange
 * minimum and maximum signed distance of the surface points from a plane.
 * Distances are positive on the side the normal points to.
 * @param p0
 * point on plane
 * @param normal
 * plane normal (needs not to be normalized)
 * @return pair of min and max distance
 */
std::pair<double,double> planeDistanceRange(
    const OFPatchSurface& surf,
    const arma::mat& p0,
    const arma::mat& normal );

std::pair<double,double> patchPlaneDistanceRange(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const arma::mat& p0,
    const arma::mat& normal,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );


struct OFPatchFaceSizeStatistics
{
  size_t nFaces = 0;
  double mean = 0, min = 0, max = 0, median = 0;
};

/**
 * @brief faceSizeStatistics
 * statistics of the face sizes in the surface.
 * The size of a face is defined as sqrt(face area).
 * The mean is the arithmetic mean over all faces (not area-weighted).
 * Throws, if the surface contains no faces.
 */
OFPatchFaceSizeStatistics faceSizeStatistics(const OFPatchSurface& surf);

OFPatchFaceSizeStatistics patchFaceSizeStatistics(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );


}

#endif // INSIGHT_OFPOLYMESHREADER_H
