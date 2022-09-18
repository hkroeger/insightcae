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

#ifndef INSIGHT_VTK_H
#define INSIGHT_VTK_H

#include <vector>
#include <map>

#include "boost_include.h"
#include <boost/concept_check.hpp>

#include "base/linearalgebra.h"

#include <limits>
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"

class vtkPolyData;


namespace insight {
  
namespace vtk {
  
typedef std::vector<arma::mat> PointList;
typedef std::vector<int> Polygon;
typedef std::vector<Polygon> PolygonList;
typedef std::tuple<Polygon,int> Cell;
typedef std::vector<Cell> CellList;

typedef std::vector<double> ScalarField;
typedef std::map<std::string, ScalarField> ScalarFieldList;
typedef std::vector<arma::mat> VectorField;
typedef std::map<std::string, VectorField> VectorFieldList;

class vtkModel
{
  
protected:
  PointList pts_;
  ScalarFieldList pointScalarFields_;
  VectorFieldList pointVectorFields_;
  ScalarFieldList cellScalarFields_;
  VectorFieldList cellVectorFields_;
  
public:
    vtkModel();
    virtual ~vtkModel();

    void setPoints(int npts, const double* x, const double* y, const double* z);
    

    int nPolyPts() const;
    
    void appendPointScalarField(const std::string& name, const double v[]);
    void appendPointVectorField(const std::string& name, const double x[], const double y[], const double z[]);
    void appendPointTensorField(const std::string& name, 
			       const double xx[], const double xy[], const double xz[],
			       const double yx[], const double yy[], const double yz[],
			       const double zx[], const double zy[], const double zz[]
			       );
    
    virtual void writeGeometryToLegacyFile(std::ostream& os) const;
    virtual void writeDataToLegacyFile(std::ostream& os) const;
    virtual void writeLegacyFile(std::ostream& os) const;
    virtual void createLegacyFile(const boost::filesystem::path& fn, bool create_dir=false) const;
};



class vtkUnstructuredGridModel
    : public vtkModel
{
  CellList cells_;

public:
    vtkUnstructuredGridModel();

    void appendCell(int nc, const int ci[], int type);
    int nCellPts() const;

    virtual void writeLegacyFile(std::ostream& os) const;
};


class vtkModel2d
: public vtkModel
{
  
protected:
  PolygonList poly_;
  
public:
    vtkModel2d();
    virtual ~vtkModel2d();

    void appendPolygon(int nc, const int ci[]);
    int nPolyPts() const;
    
    void appendCellScalarField(const std::string& name, const double s[]);
    void appendCellVectorField(const std::string& name, const double x[], const double y[], const double z[]);
    void appendCellTensorField(const std::string& name, 
			       const double xx[], const double xy[], const double xz[],
			       const double yx[], const double yy[], const double yz[],
			       const double zx[], const double zy[], const double zz[]
			      );

    virtual void writeDataToLegacyFile(std::ostream& os) const;
    virtual void writeGeometryToLegacyFile(std::ostream& os) const;
};

typedef std::shared_ptr<vtkModel2d> vtkModel2dPtr;



class ContourExtruder
{
    vtkSmartPointer<vtkPolyData> extrudedVolume_;

public:
    ContourExtruder(const arma::mat& contour, const arma::mat& dir);
    vtkSmartPointer<vtkPolyData> extrudedVolume() const;
};

}



/**
 * @brief orientNormals
 * reorients the normals, so that the mean normals point towards the point pFar
 * which is supposed to be far away from the surface
 * @param vpm
 * this data set will be modified. It is supposed to contain a field "Normals"
 * @param pFar
 * the point to which the normals should point to. It shall be far away from the surface.
 */
bool checkNormalsOrientation(vtkPolyData* vpm, const arma::mat& pFar, bool modifyNormalsFields=false);



}

#endif // INSIGHT_VTK_H
