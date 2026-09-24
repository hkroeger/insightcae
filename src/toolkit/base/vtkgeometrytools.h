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


#ifndef INSIGHT_VTKGEOMETRYTOOLS_H
#define INSIGHT_VTKGEOMETRYTOOLS_H

#include <set>
#include <map>
#include <vector>

#include <boost/filesystem.hpp>

#include "base/linearalgebra.h"
#include "vtkSmartPointer.h"

class vtkPolyData;
class vtkCellArray;
class vtkPolyDataAlgorithm;
class vtkDataSet;
class vtkDataArray;

namespace insight {

class LineMesh_to_OrderedPointTable
        : public std::vector<arma::mat>
{
    void calcConnectionInfo(vtkCellArray* lines);

public:
    typedef std::vector<vtkIdType> idList;
    typedef std::map<vtkIdType,idList> idListMap;

    idListMap pointCells_, cellPoints_;
    std::set<vtkIdType> endPoints_;

    std::vector<vtkIdType> pointIds_; // mapping of each point in ordered point table to vtkPoint index

    LineMesh_to_OrderedPointTable(vtkPolyData* pd);

    inline vtkIdType nEndpoints() const { return vtkIdType(endPoints_.size()); }

    const std::vector<vtkIdType>& pointIds() const;

    arma::mat extractOrderedData(vtkDataArray* data) const;

    void printSummary(std::ostream&, vtkPolyData* pd=nullptr) const;

    /**
     * @brief txyz
     * convert point list into a single matrix
     * @return
     * matrix with first column: distance coordinate,
     * cols 2,3,4: x, y, z
     */
    arma::mat txyz() const;
};


/**
  * return bounding box of model
  * first col: min point
  * second col: max point
  */
arma::mat STLBndBox(
  vtkSmartPointer<vtkPolyDataAlgorithm> stl_data_Set
);

/**
  * return bounding box of model
  * first col: min point
  * second col: max point
  */
arma::mat PolyDataBndBox(
  vtkSmartPointer<vtkDataSet> stl_data_Set
);

void writeSTL
(
    vtkSmartPointer<vtkPolyData> stl,
    const boost::filesystem::path& outfile
);

void writeSTL
(
   vtkSmartPointer<vtkPolyDataAlgorithm> stl,
   const boost::filesystem::path& outfile
);

}

#endif // INSIGHT_VTKGEOMETRYTOOLS_H
