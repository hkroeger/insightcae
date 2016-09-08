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

#include "vtktools.h"

#include <iostream>
#include "boost/foreach.hpp"

using namespace std;

namespace insight
{
  
namespace vtk
{

vtkModel::vtkModel()
{
}

vtkModel::~vtkModel()
{
}

void vtkModel::setPoints(int npts, const double* x, const double* y, const double* z)
{
  for (int i=0; i<npts; i++)
  {
    pts_.push_back(vec3(x[i], y[i], z[i]));
  }
}


void vtkModel::appendPointScalarField(const std::string& name, const double v[])
{
  pointScalarFields_[name]=ScalarField(v, v+pts_.size());
}

void vtkModel::appendPointVectorField(const std::string& name, const double x[], const double y[], const double z[])
{
  //cout<<"Append PointVectorField "<<name<<" to VTK"<<endl;
  pointVectorFields_[name]=VectorField();
  VectorField& vf = pointVectorFields_[name];
  for (int i=0; i<pts_.size(); i++)
  {
    //cout<<i<<" "<<x[i]<<" "<<y[i]<<" "<<z[i]<<endl;
    vf.push_back(vec3(x[i], y[i], z[i]));
  }  
}

void vtkModel::appendPointTensorField(const string& name, const double xx[], const double xy[], const double xz[], const double yx[], const double yy[], const double yz[], const double zx[], const double zy[], const double zz[])
{
  //cout<<"Append PointVectorField "<<name<<" to VTK"<<endl;
  pointVectorFields_[name]=VectorField();
  VectorField& vf = pointVectorFields_[name];
  for (int i=0; i<pts_.size(); i++)
  {
    //cout<<i<<" "<<x[i]<<" "<<y[i]<<" "<<z[i]<<endl;
    vf.push_back(tensor3(xx[i], xy[i], xz[i], yx[i], yy[i], yz[i], zx[i], zy[i], zz[i]));
  }  
}



void vtkModel::writeGeometryToLegacyFile(std::ostream& os) const
{
  os << "POINTS "<<pts_.size()<<" float"<<endl;
  BOOST_FOREACH(const arma::mat& p, pts_)
  {
    os << p(0)<<" "<<p(1)<<" "<<p(2)<<endl;
  }  
}


void vtkModel::createLegacyFile(const boost::filesystem::path& fn, bool create_dir) const
{
    if (create_dir)
    {
        if (!boost::filesystem::exists(fn.parent_path()))
            boost::filesystem::create_directories(fn.parent_path());
    }
    
    std::cout<<"Writing to "<<fn<<std::endl;
    std::ofstream f(fn.c_str());
    writeLegacyFile(f);
    f.close();
}

void vtkModel2d::appendCellScalarField(const std::string& name, const double s[])
{
  cellScalarFields_[name]=ScalarField();
  ScalarField& vf = cellScalarFields_[name];
  for (int i=0; i<poly_.size(); i++)
  {
    vf.push_back(s[i]);
  }  
}


void vtkModel2d::appendCellVectorField(const std::string& name, const double x[], const double y[], const double z[])
{
  cellVectorFields_[name]=VectorField();
  VectorField& vf = cellVectorFields_[name];
  for (int i=0; i<poly_.size(); i++)
  {
    vf.push_back(vec3(x[i], y[i], z[i]));
  }  
}

void vtkModel2d::appendCellTensorField(const std::string& name, 
			    const double xx[], const double xy[], const double xz[],
			    const double yx[], const double yy[], const double yz[],
			    const double zx[], const double zy[], const double zz[]
			  )
{
  cellVectorFields_[name]=VectorField();
  VectorField& vf = cellVectorFields_[name];
  for (int i=0; i<poly_.size(); i++)
  {
    vf.push_back(tensor3(xx[i], xy[i], xz[i], yx[i], yy[i], yz[i], zx[i], zy[i], zz[i]));
  } 
}

void vtkModel::writeDataToLegacyFile(std::ostream& os) const
{
  os<<"POINT_DATA "<<pts_.size()<<endl;
  BOOST_FOREACH(const ScalarFieldList::value_type& sf, pointScalarFields_)
  {
    os<<"SCALARS "<<sf.first<<" float 1"<<endl;
    os<<"LOOKUP_TABLE default"<<endl;
    BOOST_FOREACH(const double& v, sf.second)
    {
      os<<v<<endl;
    }
  }
  BOOST_FOREACH(const VectorFieldList::value_type& sf, pointVectorFields_)
  {
//     cout<<sf.first<<endl;
    if (pts_.size()>0)
    {
      const arma::mat& fe=sf.second[0];
      if ( (fe.n_rows==3) && (fe.n_cols==1) )
      {
	// vector 
	os<<"VECTORS "<<sf.first<<" float"<<endl;
	BOOST_FOREACH(const arma::mat& v, sf.second)
	{
	  os<<v(0)<<" "<<v(1)<<" "<<v(2)<<endl;
	}
      }
      else if ( (fe.n_rows==3) && (fe.n_cols==3) )
      {
	// tensor
	os<<"TENSORS "<<sf.first<<" float"<<endl;
	BOOST_FOREACH(const arma::mat& v, sf.second)
	{
	  os<<v(0,0)<<" "<<v(0,1)<<" "<<v(0,2)<<endl;
	  os<<v(1,0)<<" "<<v(1,1)<<" "<<v(1,2)<<endl;
	  os<<v(2,0)<<" "<<v(2,1)<<" "<<v(2,2)<<endl;
	}
      }
    }
  }
}

void vtkModel2d::writeDataToLegacyFile(std::ostream& os) const
{
  vtkModel::writeDataToLegacyFile(os);
  
  os<<"CELL_DATA "<<poly_.size()<<endl;
  BOOST_FOREACH(const ScalarFieldList::value_type& sf, cellScalarFields_)
  {
    os<<"SCALARS "<<sf.first<<" float 1"<<endl;
    os<<"LOOKUP_TABLE default"<<endl;
    BOOST_FOREACH(const double& v, sf.second)
    {
      os<<v<<endl;
    }
  }
  BOOST_FOREACH(const VectorFieldList::value_type& sf, cellVectorFields_)
  {
    if (poly_.size()>0)
    {
      const arma::mat& fe=sf.second[0];
      if ( (fe.n_rows==3) && (fe.n_cols==1) )
      {
	os<<"VECTORS "<<sf.first<<" float"<<endl;
	BOOST_FOREACH(const arma::mat& v, sf.second)
	{
	  os<<v(0)<<" "<<v(1)<<" "<<v(2)<<endl;
	}
      }
      else if ( (fe.n_rows==3) && (fe.n_cols==3) )
      {
	// tensor
	os<<"TENSORS "<<sf.first<<" float"<<endl;
	BOOST_FOREACH(const arma::mat& v, sf.second)
	{
	  os<<v(0,0)<<" "<<v(0,1)<<" "<<v(0,2)<<endl;
	  os<<v(1,0)<<" "<<v(1,1)<<" "<<v(1,2)<<endl;
	  os<<v(2,0)<<" "<<v(2,1)<<" "<<v(2,2)<<endl;
	}
      }
    }
  }
}

void vtkModel::writeLegacyFile(std::ostream& os) const
{
  os << "# vtk DataFile Version 2.0" << endl;
  os << "vtkModel2d" << endl;
  os << "ASCII" <<endl;
  os << "DATASET POLYDATA"<<endl;
  writeGeometryToLegacyFile(os);
  writeDataToLegacyFile(os);
}

vtkModel2d::vtkModel2d()
{
}

vtkModel2d::~vtkModel2d()
{
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


void vtkModel2d::writeGeometryToLegacyFile(std::ostream& os) const
{
  vtkModel::writeGeometryToLegacyFile(os);
  
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
}

}

}
