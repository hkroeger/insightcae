/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2013  hannes <email>
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
    
    virtual void writeGeometryToLegacyFile(std::ostream& os) const;
};

}

}

#endif // INSIGHT_VTK_H
