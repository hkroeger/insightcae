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


#include "vtkgeometrytools.h"
#include "base/exception.h"

#include <cmath>

#include "vtkSTLWriter.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkDataSet.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkDataArray.h"

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost::filesystem;

namespace insight
{


void LineMesh_to_OrderedPointTable::calcConnectionInfo(vtkCellArray* lines)
{
    pointCells_.clear();
    cellPoints_.clear();
    endPoints_.clear();

    lines->InitTraversal();
    vtkIdType npts=-1;

#if (VTK_MAJOR_VERSION>=8) || ( (VTK_MAJOR_VERSION==8) && (VTK_MINOR_VERSION>2))
    const
#endif
    vtkIdType *pt=nullptr;

    for (vtkIdType i=0; lines->GetNextCell(npts, pt); i++)
      {

        idList idl;

        for (vtkIdType j=0; j<npts; j++)
          {
            idl.push_back(pt[j]);
            pointCells_[pt[j]].push_back(i);
          }

        cellPoints_[i]=idl;
      }


    for (vtkIdType i=0; i<vtkIdType(pointCells_.size()); i++)
    {
        const idList& pc=pointCells_[i];
        if (pc.size()==1) endPoints_.insert(i);
    }

}


LineMesh_to_OrderedPointTable::LineMesh_to_OrderedPointTable(vtkPolyData* pd)
{
    vtkCellArray* lines = pd->GetLines();

    // find min element length
    double L=0.;
    int nL=0;
    {
        lines->InitTraversal();
        vtkIdType npts=-1;
#if (VTK_MAJOR_VERSION>=8) || ( (VTK_MAJOR_VERSION==8) && (VTK_MINOR_VERSION>2))
        const
#endif
        vtkIdType *pt=nullptr;
        for (int i=0; lines->GetNextCell(npts, pt); i++)
          {
            if (npts==2)
            {
                double p1[3], p2[3];
                pd->GetPoint(pt[0], p1);
                pd->GetPoint(pt[1], p2);
                L+=sqrt( pow(p1[0]-p2[0],2) + pow(p1[1]-p2[1],2) + pow(p1[2]-p2[2],2) );
                nL++;
            }
          }
    }
    L/=double(nL);


    double tol=0.5*L;


    // Extract connection info
    //    std::cout<<"tol="<<tol<<std::endl;
    calcConnectionInfo(lines);
    printSummary(std::cout, pd);

    typedef std::map<vtkIdType,vtkIdType> AddLinesList;
    AddLinesList addLines;
    for (vtkIdType i: endPoints_)
    {
        double p1[3];
        pd->GetPoint(i, p1);

        double ldist=1e100;
        vtkIdType lj=-1;

        for (vtkIdType j: endPoints_)
        {
            if (i!=j)
            {
                double p2[3];
                pd->GetPoint(j, p2);

                double dist = sqrt( pow(p1[0]-p2[0],2) + pow(p1[1]-p2[1],2) + pow(p1[2]-p2[2],2) );

                if ( dist < tol )
                {
                    if (dist<ldist)
                    {
                        ldist=dist;
                        lj=j;
                    }
                }
            }
        }

        if (lj>=0)
        {
            vtkIdType li=i;
            if (li>lj) std::swap(li,lj);
            addLines[li]=lj;
//            std::cout<<"add line "<<li<<" => "<<lj<<std::endl;
        }
    }

    for (const AddLinesList::value_type& al: addLines)
    {
        vtkIdType eps[2];
        eps[0]=al.first;
        eps[1]=al.second;
        pd->InsertNextCell(VTK_LINE, 2, eps);
    }

    lines = pd->GetLines();
    calcConnectionInfo(lines);

    vtkIdType id_p0=0;
    if (endPoints_.size()>0)
        id_p0=*endPoints_.begin();

    std::set<vtkIdType> visitedCells;

    // ordered list of points (polyline)
    vtkIdType cid=id_p0;
    double xyz[3];

    pd->GetPoint(cid, xyz);
    pointIds_.push_back(cid);
    this->push_back(vec3(xyz[0], xyz[1], xyz[2]));

    do
    {
      idList pc = pointCells_[cid];
      if (visitedCells.find(pc[0]) == visitedCells.end())
        {
          const idList& pts = cellPoints_[pc[0]];
          if (pts[0]!=cid) cid=pts[0];
          else if (pts[1]!=cid) cid=pts[1];
          visitedCells.insert(pc[0]);
//          std::cout<<"visited a) "<<pc[0]<<std::endl;
        }
      else if (visitedCells.find(pc[1]) == visitedCells.end())
        {
          const idList& pts = cellPoints_[pc[1]];
          if (pts[0]!=cid) cid=pts[0];
          else if (pts[1]!=cid) cid=pts[1];
          visitedCells.insert(pc[1]);
//          std::cout<<"visited b) "<<pc[1]<<std::endl;
        }
      else
      {
//          std::cout<<"break"<<std::endl;
          break;
      }

      pd->GetPoint(cid, xyz);
      pointIds_.push_back(cid);
      this->push_back(vec3(xyz[0], xyz[1], xyz[2]));

    } while (visitedCells.size()<cellPoints_.size());
}

const std::vector<vtkIdType>& LineMesh_to_OrderedPointTable::pointIds() const
{
    return pointIds_;
}

arma::mat LineMesh_to_OrderedPointTable::extractOrderedData(vtkDataArray* data) const
{
    arma::mat result = arma::zeros( pointIds().size(), data->GetNumberOfComponents() );
    for(size_t i=0; i<pointIds().size(); ++i)
    {
        double pd[data->GetNumberOfComponents()];
        data->GetTuple( pointIds()[i], pd );
        for (int k=0;k<data->GetNumberOfComponents();++k)
            result(i, k)=pd[k];
    }
    return result;
}

void LineMesh_to_OrderedPointTable::printSummary(std::ostream& os, vtkPolyData* pd) const
{
    os<<"# points : "<<size()<<std::endl;
    os<<"# endpoints : "<<endPoints_.size()<<std::endl;
    for (vtkIdType i: endPoints_)
    {
        os<<"   "<<i;
        if (pd)
        {
            double p[3];
            pd->GetPoint(i, p);
            os<<" @ ("<<p[0]<<", "<<p[1]<<", "<<p[2]<<")";
        }
        os <<std::endl;
    }
    if (size()>=2)
    {
        os<<"first/last point : "<<std::endl;
        {
            const arma::mat& p = *begin();
            os<<" ("<<p(0)<<", "<<p(1)<<", "<<p(2)<<")";
        }
        os<<" ...\n";
        {
            const arma::mat& p = back();
            os<<" ("<<p(0)<<", "<<p(1)<<", "<<p(2)<<")";
        }
    }
    os<<"\n\n";
}

arma::mat LineMesh_to_OrderedPointTable::txyz() const
{
    arma::mat res = arma::zeros(size(), 4);
    double t=0;
    for (size_t i=0; i<size(); i++)
    {
        const arma::mat& p=(*this)[i];

        if (i>0)
        {
            t+=arma::norm( p-(*this)[i-1], 2 );
        }

        res(i,0)=t;
        res(i,1)=p(0);
        res(i,2)=p(1);
        res(i,3)=p(2);

    }
    return res;
}


arma::mat STLBndBox
(
  vtkSmartPointer<vtkPolyDataAlgorithm> in
)
{
  CurrentExceptionContext ec("Computing bounding box of VTK poly data set");

  in->Update();
  return PolyDataBndBox(in->GetOutput());
}

arma::mat PolyDataBndBox
(
  vtkSmartPointer<vtkDataSet> in
)
{
  double bb[6];
  in->GetBounds(bb);

  arma::mat bbm = ArmaMatCmpts{
      { bb[0], bb[1] },
      { bb[2], bb[3] },
      { bb[4], bb[5] }
  };

  return bbm;
}


void writeSTL
(
    vtkSmartPointer<vtkPolyData> stl,
    const boost::filesystem::path& outfile
)
{
    CurrentExceptionContext ec("Writing STL mesh to file "+outfile.string());

    std::string file_ext = outfile.filename().extension().string();
    boost::to_lower(file_ext);

    vtkSmartPointer<vtkSTLWriter> sw = vtkSmartPointer<vtkSTLWriter>::New();
    sw->SetInputData(stl);
    sw->SetFileName(outfile.string().c_str());
    if (file_ext==".stlb")
    {
        sw->SetFileTypeToBinary();
    }
    else
    {
        sw->SetFileTypeToASCII();
    }
    sw->Update();
}



void writeSTL
(
   vtkSmartPointer<vtkPolyDataAlgorithm> stl,
   const boost::filesystem::path& outfile
)
{
  CurrentExceptionContext ec("Writing STL mesh to file "+outfile.string());

  std::string file_ext = outfile.filename().extension().string();
  boost::to_lower(file_ext);

  vtkSmartPointer<vtkSTLWriter> sw = vtkSmartPointer<vtkSTLWriter>::New();
  sw->SetInputConnection(stl->GetOutputPort());
  sw->SetFileName(outfile.string().c_str());
  if (file_ext==".stlb")
  {
    sw->SetFileTypeToBinary();
  }
  else
  {
    sw->SetFileTypeToASCII();
  }
  sw->Update();
}


}
