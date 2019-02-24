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
 */

#include "tools.h"
#include <fstream>
#include <cstdlib>
#include <dlfcn.h>

#include "base/boost_include.h"
#include "base/exception.h"

#include "vtkSTLReader.h"
#include "vtkSTLWriter.h"
#include "vtkPlane.h"
#include "vtkCutter.h"
#include "vtkSmartPointer.h"
#include "vtkClipPolyData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;


namespace insight
{


TemporaryCaseDir::TemporaryCaseDir(bool keep, const std::string& prefix)
: keep_(keep)
{
  if (getenv("INSIGHT_KEEPTEMPCASEDIR"))
    keep_=true;
  dir = unique_path(prefix+"%%%%%%%");
  create_directories(dir);
}

TemporaryCaseDir::~TemporaryCaseDir()
{
  if (!keep_)
    remove_all(dir);
}


SharedPathList::SharedPathList()
{
  char *var_usershareddir=getenv("INSIGHT_USERSHAREDDIR");
  char *var_globalshareddir=getenv("INSIGHT_GLOBALSHAREDDIRS");
  
  if (var_usershareddir) 
  {
    push_back(var_usershareddir);
  }
  else
  {
    char *userdir=getenv("HOME");
    if (userdir)
    {
      push_back( path(userdir)/".insight"/"share" );
    }
  }
  
  if (var_globalshareddir) 
  {
    std::vector<string> globals;
    split(globals, var_globalshareddir, is_any_of(":"));
    for (const string& s: globals) push_back(s);
  }
  else
  {
    push_back( path("/usr/share/insight") );
  }
}

SharedPathList::~SharedPathList()
{
}


path SharedPathList::getSharedFilePath(const path& file)
{
  BOOST_REVERSE_FOREACH( const path& p, *this)
  {
    if (exists(p/file)) 
      return p/file;
  }
  
  // nothing found
  throw insight::Exception(
        std::string("Requested shared file ")
         +file.c_str()
         +" not found either in global nor user shared directories"
        );
}

void SharedPathList::insertIfNotPresent(const path& spr)
{
  path sp = boost::filesystem::absolute(spr);
  if (std::find(begin(), end(), sp) == end())
  {
    std::cout<<"Extend search path: "<<sp.string()<<std::endl;
    push_back(sp);
  }
  else
  {
    std::cout<<"Already included in search path: "<<sp.string()<<std::endl;
  }
}

void SharedPathList::insertFileDirectoyIfNotPresent(const path& sp)
{
  if (boost::filesystem::is_directory(sp))
  {
    insertIfNotPresent(sp);
  }
  else
  {
    insertIfNotPresent(sp.parent_path());
  }
}




SharedPathList SharedPathList::searchPathList;




ExecTimer::ExecTimer(const std::string& name)
: boost::timer::auto_cpu_timer(boost::timer::default_places, name+": END %ws wall, %us usr + %ss sys = %ts CPU (%p%)\n")
{
    std::cout<< ( name+": BEGIN\n" );
}


ExecTimer::~ExecTimer()
{}




void LineMesh_to_OrderedPointTable::calcConnectionInfo(vtkCellArray* lines)
{
    pointCells_.clear();
    cellPoints_.clear();
    endPoints_.clear();

    lines->InitTraversal();
    vtkIdType npts=-1;
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
//    std::cout<<"#lines="<<lines->GetNumberOfCells()<<std::endl;

    // find min element length
    double L=0.;
    int nL=0;
    {
        lines->InitTraversal();
        vtkIdType npts=-1;
        vtkIdType *pt=nullptr;
        for (int i=0; lines->GetNextCell(npts, pt); i++)
          {
            if (npts==2)
            {
                double p1[3], p2[3];
                pd->GetPoint(pt[0], p1);
                pd->GetPoint(pt[1], p2);
//                L=std::min
//                (
//                    L,
//                    sqrt( pow(p1[0]-p2[0],2) + pow(p1[1]-p2[1],2) + pow(p1[2]-p2[2],2) )
//                );
                L+=sqrt( pow(p1[0]-p2[0],2) + pow(p1[1]-p2[1],2) + pow(p1[2]-p2[2],2) );
                nL++;
            }
          }
    }
    L/=double(nL);


    double tol=0.5*L;
//    std::cout<<"tol="<<tol<<std::endl;



    // Extract connection info
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
            std::cout<<"add line "<<li<<" => "<<lj<<std::endl;
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
      this->push_back(vec3(xyz[0], xyz[1], xyz[2]));

    } while (visitedCells.size()<cellPoints_.size());
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


vtk_Transformer::~vtk_Transformer()
{}

vtkSmartPointer<vtkPolyDataAlgorithm>
readSTL
(
  const boost::filesystem::path& path,
  const vtk_TransformerList& trsf
)
{
  CurrentExceptionContext ce("Reading STL file using VTK reader");

  if (!boost::filesystem::exists(path))
    throw insight::Exception("file "+path.string()+" does not exist!");

  vtkSmartPointer<vtkSTLReader> stl = vtkSmartPointer<vtkSTLReader>::New();
  stl->SetFileName(path.c_str());

  std::vector<vtkSmartPointer<vtkPolyDataAlgorithm> > imr;
  vtkSmartPointer<vtkPolyDataAlgorithm> i0=stl;
  for (const auto& t: trsf)
  {
    imr.push_back(vtkSmartPointer<vtkPolyDataAlgorithm>( t->apply_VTK_Transform(i0) ));
    i0=imr.back();
  }

  return i0;
}


arma::mat STLBndBox
(
  vtkSmartPointer<vtkPolyDataAlgorithm> in
)
{

  double bb[6];
  in->Update();
  in->GetOutput()->GetBounds(bb);

  arma::mat bbm;
  bbm
    << bb[0] << bb[1] << arma::endr
    << bb[2] << bb[3] << arma::endr
    << bb[4] << bb[5] << arma::endr;

  return bbm;
}



}
