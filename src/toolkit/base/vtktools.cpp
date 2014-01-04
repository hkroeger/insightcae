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

#include "vtktools.h"

#include <iostream>
#include "boost/foreach.hpp"

using namespace std;

namespace insight
{
  
namespace vtk
{

vtkModel2d::vtkModel2d()
{
}

vtkModel2d::~vtkModel2d()
{

}

void vtkModel2d::setPoints(int npts, const double* x, const double* y, const double* z)
{
  for (int i=0; i<npts; i++)
  {
    pts_.push_back(vec3(x[i], y[i], z[i]));
  }
}

void vtkModel2d::appendPolygon(int nc, const int ci[])
{
  poly_.push_back(Polygon(ci, ci+nc));
}

int vtkModel2d::nPolyPts() const
{
  int n=0;
  BOOST_FOREACH(const Polygon& p, poly_)
  {
    n+=p.size()+1;
  }
  return n;
}

void vtkModel2d::appendPointScalarField(const std::string& name, const double v[])
{
  pointScalarFields_[name]=ScalarField(v, v+pts_.size());
}

void vtkModel2d::appendPointVectorField(const std::string& name, const double x[], const double y[], const double z[])
{
  pointVectorFields_[name]=VectorField();
  VectorField& vf = pointVectorFields_[name];
  for (int i=0; i<pts_.size(); i++)
  {
    vf.push_back(vec3(x[i], y[i], z[i]));
  }  
}

void vtkModel2d::writeLegacyFile(std::ostream& os) const
{
  os << "# vtk DataFile Version 2.0" << endl;
  os << "vtkModel2d" << endl;
  os << "ASCII" <<endl;
  os << "DATASET POLYDATA"<<endl;
  os << "POINTS "<<pts_.size()<<" float"<<endl;
  BOOST_FOREACH(const arma::mat& p, pts_)
  {
    os << p(0)<<" "<<p(1)<<" "<<p(2)<<endl;
  }
  os<<"POLYGONS "<<poly_.size()<<" "<<nPolyPts()<<endl;
  BOOST_FOREACH(const Polygon& p, poly_)
  {
    os<<p.size();
    BOOST_FOREACH(const int& i, p)
    {
      os<<" "<<i;
    }
    os<<endl;
  }

  os<<"POINT_DATA "<<pts_.size()<<endl;
  BOOST_FOREACH(const ScalarFieldList::value_type& sf, pointScalarFields_)
  {
    os<<"SCALARS "<<sf.first<<" float 1"<<endl;
    BOOST_FOREACH(const double& v, sf.second)
    {
      os<<v<<endl;
    }
  }
  BOOST_FOREACH(const VectorFieldList::value_type& sf, pointVectorFields_)
  {
    os<<"VECTORS "<<sf.first<<" float"<<endl;
    BOOST_FOREACH(const arma::mat& v, sf.second)
    {
      os<<v(0)<<" "<<v(1)<<" "<<v(2)<<endl;
    }
  }
}

}

}