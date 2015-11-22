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
#include <boost/concept_check.hpp>

#include "base/linearalgebra.h"

namespace insight {
  
namespace vtk {
  
typedef std::vector<arma::mat> PointList;
typedef std::vector<int> Polygon;
typedef std::vector<Polygon> PolygonList;

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
    
    void appendPolygon(int nc, const int ci[]);
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

}

}

#endif // INSIGHT_VTK_H
