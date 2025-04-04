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

#include "base/exception.h"

#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"

#include "vtkDelaunay2D.h"
#include "vtkLinearExtrusionFilter.h"

#include "vtkDataSet.h"
#include "vtkAppendFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkMultiBlockDataSet.h"

#include "base/cppextensions.h"

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
  for (size_t i=0; i<pts_.size(); i++)
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
  for (size_t i=0; i<pts_.size(); i++)
  {
    //cout<<i<<" "<<x[i]<<" "<<y[i]<<" "<<z[i]<<endl;
    vf.push_back(tensor3(xx[i], xy[i], xz[i], yx[i], yy[i], yz[i], zx[i], zy[i], zz[i]));
  }  
}



void vtkModel::writeGeometryToLegacyFile(std::ostream& os) const
{
  os << "POINTS "<<pts_.size()<<" float"<<endl;
  for (const arma::mat& p: pts_)
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




vtkUnstructuredGridModel::vtkUnstructuredGridModel()
{
}

void vtkUnstructuredGridModel::appendCell(int nc, const int ci[], int type)
{
  cells_.push_back( Cell(Polygon(ci, ci+nc), type) );
}

int vtkUnstructuredGridModel::nCellPts() const
{
  int n=0;
  for (const Cell& c: cells_)
  {
    n+=std::get<0>(c).size()+1;
  }
  return n;
}

void vtkUnstructuredGridModel::writeLegacyFile(std::ostream& os) const
{
  os << "# vtk DataFile Version 2.0" << endl;
  os << "vtkUnstructuredGridModel" << endl;
  os << "ASCII" <<endl;
  os << "DATASET UNSTRUCTURED_GRID"<<endl;
  writeGeometryToLegacyFile(os);
  os<<"CELLS "<<cells_.size()<<" "<<nCellPts()<<endl;
  for (const Cell& c: cells_)
  {
    const Polygon& p = std::get<0>(c);
    os<<p.size();
    for (const int& i: p)
    {
      os<<" "<<i;
    }
    os<<endl;
  }
  os<<"CELL_TYPES "<<cells_.size()<<endl;
  for (const Cell& c: cells_)
  {
    os<<std::get<1>(c)<<endl;
  }
  writeDataToLegacyFile(os);
}

void vtkModel2d::appendCellScalarField(const std::string& name, const double s[])
{
  cellScalarFields_[name]=ScalarField();
  ScalarField& vf = cellScalarFields_[name];
  for (size_t i=0; i<poly_.size(); i++)
  {
    vf.push_back(s[i]);
  }  
}


void vtkModel2d::appendCellVectorField(const std::string& name, const double x[], const double y[], const double z[])
{
  cellVectorFields_[name]=VectorField();
  VectorField& vf = cellVectorFields_[name];
  for (size_t i=0; i<poly_.size(); i++)
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
  for (size_t i=0; i<poly_.size(); i++)
  {
    vf.push_back(tensor3(xx[i], xy[i], xz[i], yx[i], yy[i], yz[i], zx[i], zy[i], zz[i]));
  } 
}

void vtkModel::writeDataToLegacyFile(std::ostream& os) const
{
  os<<"POINT_DATA "<<pts_.size()<<endl;
  for (const ScalarFieldList::value_type& sf: pointScalarFields_)
  {
    os<<"SCALARS "<<sf.first<<" float 1"<<endl;
    os<<"LOOKUP_TABLE default"<<endl;
    for (const double& v: sf.second)
    {
      os<<v<<endl;
    }
  }
  for (const VectorFieldList::value_type& sf: pointVectorFields_)
  {
//     cout<<sf.first<<endl;
    if (pts_.size()>0)
    {
      const arma::mat& fe=sf.second[0];
      if ( (fe.n_rows==3) && (fe.n_cols==1) )
      {
	// vector 
	os<<"VECTORS "<<sf.first<<" float"<<endl;
	for (const arma::mat& v: sf.second)
	{
	  os<<v(0)<<" "<<v(1)<<" "<<v(2)<<endl;
	}
      }
      else if ( (fe.n_rows==3) && (fe.n_cols==3) )
      {
	// tensor
	os<<"TENSORS "<<sf.first<<" float"<<endl;
	for (const arma::mat& v: sf.second)
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
  for (const ScalarFieldList::value_type& sf: cellScalarFields_)
  {
    os<<"SCALARS "<<sf.first<<" float 1"<<endl;
    os<<"LOOKUP_TABLE default"<<endl;
    for (const double& v: sf.second)
    {
      os<<v<<endl;
    }
  }
  for (const VectorFieldList::value_type& sf: cellVectorFields_)
  {
    if (poly_.size()>0)
    {
      const arma::mat& fe=sf.second[0];
      if ( (fe.n_rows==3) && (fe.n_cols==1) )
      {
	os<<"VECTORS "<<sf.first<<" float"<<endl;
	for (const arma::mat& v: sf.second)
	{
	  os<<v(0)<<" "<<v(1)<<" "<<v(2)<<endl;
	}
      }
      else if ( (fe.n_rows==3) && (fe.n_cols==3) )
      {
	// tensor
	os<<"TENSORS "<<sf.first<<" float"<<endl;
	for (const arma::mat& v: sf.second)
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
  for (const Polygon& p: poly_)
  {
    n+=p.size()+1;
  }
  return n;
}


void vtkModel2d::writeGeometryToLegacyFile(std::ostream& os) const
{
  vtkModel::writeGeometryToLegacyFile(os);
  
  os<<"POLYGONS "<<poly_.size()<<" "<<nPolyPts()<<endl;
  for (const Polygon& p: poly_)
  {
    os<<p.size();
    for (const int& i: p)
    {
      os<<" "<<i;
    }
    os<<endl;
  }
}




ContourExtruder::ContourExtruder(const arma::mat &contour, const arma::mat& dir)
{
    auto inputpts = vtkSmartPointer<vtkPolyData>::New();
    auto pts=vtkSmartPointer<vtkPoints>::New();
    for (arma::uword i=0; i<contour.n_rows; ++i)
    {
        pts->InsertNextPoint( arma::mat(contour.row(i)).memptr() );
    }
    inputpts->SetPoints(pts);

    auto surf = vtkSmartPointer<vtkDelaunay2D>::New();
    surf->SetInputData(inputpts);

    auto extr = vtkSmartPointer<vtkLinearExtrusionFilter>::New();
    extr->SetInputConnection(surf->GetOutputPort());
    extr->SetExtrusionTypeToVectorExtrusion();
    extr->SetVector(const_cast<double*>(dir.memptr()));

    extr->Update();

    extrudedVolume_ = extr->GetOutput();
}

vtkSmartPointer<vtkPolyData> ContourExtruder::extrudedVolume() const
{
    return extrudedVolume_;
}





}


bool checkNormalsOrientation(vtkPolyData* vpm, const arma::mat& pFar, bool modifyNormalsFields)
{
  auto* pn = vpm->GetPointData()->GetArray("Normals");
  insight::assertion(pn!=nullptr, "the input data does not contain a field \"Normals\"!");

  bool toBeInverted=false;

  if (pn->GetNumberOfTuples()>0) // skip empty mesh
  {
    // calc mean normals and CoG
    arma::mat meanN, CoG;
    meanN=CoG=arma::zeros(3);

    insight::assertion(pn->GetNumberOfTuples()==vpm->GetNumberOfPoints(),
                       "internal error: number of point normals not equal to number of points!");

    for (vtkIdType i=0; i<pn->GetNumberOfTuples(); ++i)
    {
      arma::mat n(pn->GetTuple3(i), 3, 1);
      meanN += n;
      arma::mat p(vpm->GetPoint(i), 3, 1);
      CoG += p;
    }
    meanN /= double(pn->GetNumberOfTuples());
    CoG /= double(vpm->GetNumberOfPoints());

    // check side of pFar
    arma::mat dir = pFar-CoG;
    double ldir=arma::norm(dir,2);
    insight::assertion(ldir>1e-10, "pFar must not be identical with the CoG of the model points!");
    dir/=ldir;

    double Lmean=arma::norm(meanN, 2);
    insight::assertion(Lmean>1e-10, "mean normal vector is zero!");
    meanN /= Lmean;

    toBeInverted = arma::dot(dir, meanN)<0;

    if (!modifyNormalsFields)
      return toBeInverted;

    if (toBeInverted)
    {
      // invert point normals (and, if present also cell normals)

      std::vector<vtkDataArray*> normalArraysToBeInverted = {pn};
      if (auto* cn = vpm->GetCellData()->GetArray("Normals"))
      {
        normalArraysToBeInverted.push_back(cn);
      }

      for (vtkDataArray* narray: normalArraysToBeInverted)
      {
        for (vtkIdType i=0; i<narray->GetNumberOfTuples(); ++i)
        {
          double n[3];
          narray->GetTuple(i, n);
          for (int i=0; i<3; i++) n[i]*=-1.;
          narray->SetTuple(i, n);
        }
      }
    }
  }

  return toBeInverted;
}


vtkSmartPointer<vtkUnstructuredGrid> multiBlockDataSetToUnstructuredGrid(vtkDataObject *input)
{
    auto output = vtkSmartPointer<vtkUnstructuredGrid>::New();

    bool MergePoints = true;
    vtkIdType SubTreeCompositeIndex = 0;
    double Tolerance = 0.;

    auto AddDataSet = [](vtkDataSet* ds, vtkAppendFilter* appender)
    {
      vtkDataSet* clone = ds->NewInstance();
      clone->ShallowCopy(ds);
      appender->AddInputData(clone);
      clone->Delete();
    };

    auto ExecuteSubTree = [](
      vtkCompositeDataSet* curCD, vtkAppendFilter* appender)
    {
      vtkCompositeDataIterator* iter2 = curCD->NewIterator();
      for (iter2->InitTraversal(); !iter2->IsDoneWithTraversal(); iter2->GoToNextItem())
      {
        vtkDataSet* curDS = vtkDataSet::SafeDownCast(iter2->GetCurrentDataObject());
        if (curDS)
        {
          appender->AddInputData(curDS);
        }
      }
      iter2->Delete();
    };



    vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(input);
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);

    if (ug)
    {
      output->ShallowCopy(ug);
    }
    else
    {
      auto appender = vtkSmartPointer<vtkAppendFilter>::New();
      appender->SetMergePoints(MergePoints ? 1 : 0);
      if (MergePoints)
      {
#if (VTK_MAJOR_VERSION>8 || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=90) )
        appender->SetTolerance(Tolerance);
#endif
      }
      if (ds)
      {
        AddDataSet(ds, appender);
      }
      else if (cd)
      {
        if (SubTreeCompositeIndex == 0)
        {
          ExecuteSubTree(cd, appender);
        }
        vtkDataObjectTreeIterator* iter = vtkDataObjectTreeIterator::SafeDownCast(cd->NewIterator());
        if (!iter)
        {
          throw insight::Exception("Composite data is not a tree");
        }
        iter->VisitOnlyLeavesOff();
        for (iter->InitTraversal();
             !iter->IsDoneWithTraversal() && iter->GetCurrentFlatIndex() <= SubTreeCompositeIndex;
             iter->GoToNextItem())
        {
          if (iter->GetCurrentFlatIndex() == SubTreeCompositeIndex)
          {
            vtkDataObject* curDO = iter->GetCurrentDataObject();
            vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
            vtkUnstructuredGrid* curUG = vtkUnstructuredGrid::SafeDownCast(curDO);
            vtkDataSet* curDS = vtkUnstructuredGrid::SafeDownCast(curDO);
            if (curUG)
            {
              output->ShallowCopy(curUG);
              // NOTE: Not using the appender at all.
            }
            else if (curDS && curCD->GetNumberOfPoints() > 0)
            {
              AddDataSet(curDS, appender);
            }
            else if (curCD)
            {
              ExecuteSubTree(curCD, appender);
            }
            break;
          }
        }
        iter->Delete();
      }

      if (appender->GetNumberOfInputConnections(0) > 0)
      {
        appender->Update();
        output->ShallowCopy(appender->GetOutput());
        // this will override field data the vtkAppendFilter passed from the first
        // block. It seems like a reasonable approach, if global field data is
        // present.
        if (ds)
        {
          output->GetFieldData()->PassData(ds->GetFieldData());
        }
        else if (cd)
        {
          output->GetFieldData()->PassData(cd->GetFieldData());
        }

      }
//          RemovePartialArrays(output);
    }

    return output;
}



size_t computeObjectSize(vtkSmartPointer<vtkPolyData> pd)
{
    return pd->GetActualMemorySize()*1024;
}


size_t computeObjectSize(vtkSmartPointer<vtkPointSet> pd)
{
    return pd->GetActualMemorySize()*1024;
}

size_t computeObjectSize(vtkSmartPointer<vtkDataObject> pd)
{
    return pd->GetActualMemorySize()*1024;
}

arma::mat vec3(vtkPoints *pts, int i)
{
    return vec3FromComponents(pts->GetPoint(i));
}

arma::mat vec3(vtkPointSet *pts, int i)
{
    return vec3FromComponents(pts->GetPoint(i));
}

}

namespace std {

std::size_t hash<vtkSmartPointer<vtkDataObject> >::operator()
    (const vtkSmartPointer<vtkDataObject>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<size_t>()(v->GetActualMemorySize()));
//    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
//    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfCells()));
//    auto np=v->GetNumberOfPoints();
//    auto step=std::max<vtkIdType>(1, np/4);
//    for (vtkIdType i=0; i<np; i+=step)
//    {
//      double x[3];
//      v->GetPoint(i, x);
//      for (int j=0; j<3; ++j)
//        std::hash_combine(h, std::hash<double>()(x[j]));
//    }
    return h;
}

std::size_t hash<vtkSmartPointer<vtkPolyData> >::operator()
    (const vtkSmartPointer<vtkPolyData>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfCells()));
    auto np=v->GetNumberOfPoints();
    auto step=std::max<vtkIdType>(1, np/4);
    for (vtkIdType i=0; i<np; i+=step)
    {
      double x[3];
      v->GetPoint(i, x);
      for (int j=0; j<3; ++j)
        std::hash_combine(h, std::hash<double>()(x[j]));
    }
    return h;
}

std::size_t hash<vtkSmartPointer<vtkPointSet> >::operator()
    (const vtkSmartPointer<vtkPointSet>& v) const
{
    size_t h=0;
    std::hash_combine(h, std::hash<vtkIdType>()(v->GetNumberOfPoints()));
    auto np=v->GetNumberOfPoints();
    auto step=std::max<vtkIdType>(1, np/4);
    for (vtkIdType i=0; i<np; i+=step)
    {
      double x[3];
      v->GetPoint(i, x);
      for (int j=0; j<3; ++j)
        std::hash_combine(h, std::hash<double>()(x[j]));
    }
    return h;
}

}
