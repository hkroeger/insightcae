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

#include "sketch.h"
#include "base/exception.h"
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "base/tools.h"

#include "datum.h"

#include "TColStd_Array1OfInteger.hxx"
#include "GC_MakeArcOfCircle.hxx"
#include "ShapeAnalysis_FreeBounds.hxx"

#include <chrono>

using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


namespace insight {
namespace cad {

std::string DXFReader::curLayerName()
{
  return getAttributes().getLayer();
}

bool DXFReader::notFiltered()
{
  return boost::regex_match( curLayerName(), layerpattern_ );
}

DXFReader::DXFReader(
        const boost::filesystem::path& filename,
        const std::string& layerpattern )
: layerpattern_(layerpattern)
{
  std::unique_ptr<DL_Dxf> dxf(new DL_Dxf());
  if (!dxf->in(filename.string(), this))
  {
    std::cerr << filename << " could not be opened." << std::endl;
  }
}

DXFReader::~DXFReader()
{
}

void DXFReader::addArc(const DL_ArcData &a)
{
  insight::dbg()<<"add arc, layer "<<getAttributes().getLayer()<<std::endl;
  if (notFiltered())
  {
    gp_Pnt cp(a.cx, a.cy, a.cz);
    gp_Circ c = gce_MakeCirc( gp_Ax2(cp, gp_Dir(0,0,1)), a.radius );
    gp_Pnt p0(cp); p0.Translate(gp_Vec(a.radius*::cos(M_PI*a.angle1/180.), a.radius*::sin(M_PI*a.angle1/180.), 0) );
    gp_Pnt p1(cp); p1.Translate(gp_Vec(a.radius*::cos(M_PI*a.angle2/180.), a.radius*::sin(M_PI*a.angle2/180.), 0) );
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c, p0, p1).Edge();
    ls_[curLayerName()].Append(e);
  }
  else
      insight::dbg()<<"=> filtered ("<<layerpattern_<<")"<<std::endl;
}

void DXFReader::addLine (const DL_LineData &l)
{
  insight::dbg()<<"add line, layer "<<getAttributes().getLayer()<<std::endl;
  if (notFiltered())
  {
    gp_Pnt p0(l.x1, l.y1, l.z1);
    gp_Pnt p1(l.x2, l.y2, l.z2);
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    ls_[curLayerName()].Append(e);
  }
  else
      insight::dbg()<<"=> filtered ("<<layerpattern_<<")"<<std::endl;
}

void DXFReader::addPolyline(const DL_PolylineData &pl)
{
  insight::dbg()<<"add polyline, layer "<<getAttributes().getLayer()<<std::endl;
  if (notFiltered())
  {
    pl_.reset(new Polyline(*this, curLayerName()));
    pl_->closed=false;

    pl_->lp.reset();

    if (pl.flags & 1)
    {
      pl_->p0.reset();
      pl_->closed=true;
    }
  }
  else
  {
    insight::dbg()<<"=> filtered ("<<layerpattern_<<")"<<std::endl;
    pl_.reset();
  }
}

DXFReader::Polyline::Polyline(DXFReader& r, const std::string& layername)
: reader(r), layername_(layername), lbulge(0.0)
{
}


DXFReader::Polyline::~Polyline()
{
    if ( closed )
    {
      DL_VertexData pv;
      pv.x=p0->X();
      pv.y=p0->Y();
      pv.z=p0->Z();
      pv.bulge=0;
      reader.addVertexPolyLine(pv, *this);
    }
}



void DXFReader::addVertex(const DL_VertexData &pv)
{
  if (notFiltered())
  {
    addVertexPolyLine(pv, *pl_);
  }
}

void DXFReader::addVertexPolyLine(const DL_VertexData & pv, Polyline& pl)
{
    gp_Pnt p(pv.x, pv.y, pv.z);

    if (!pl.p0)
    {
      pl.p0.reset(new gp_Pnt(p));
    }
    else
    {
      double bulge=pl.lbulge;
      if (fabs(bulge)<1e-10)
      {
        TopoDS_Edge e=BRepBuilderAPI_MakeEdge(*pl.lp, p).Edge();
        ls_[pl.layername_].Append(e);
      }
      else
      {
        gp_XYZ pa(pl.lp->XYZ());
        gp_XYZ pb(p.XYZ());
        double u=(pb-pa).Modulus();
        double i=bulge*u/2.0;
        gp_XYZ er=(pb-pa).Crossed(gp_XYZ(0,0,1)).Normalized();
        gp_XYZ pt( pa + 0.5*(pb-pa) + i*er );
        TopoDS_Edge e=BRepBuilderAPI_MakeEdge(
          GC_MakeArcOfCircle(gp_Pnt(pa), gp_Pnt(pt), gp_Pnt(pb)).Value(),
          gp_Pnt(pa), gp_Pnt(pb)
        ).Edge();
        ls_[pl.layername_].Append(e);
      }
    }
    pl.lp.reset(new gp_Pnt(p));
    pl.lbulge=pv.bulge;

}



void DXFReader::addSpline(const DL_SplineData& sp)
{
  insight::dbg()<<"add spline, layer "<<getAttributes().getLayer()<<std::endl;
  if (notFiltered())
  {
    spl_deg_=sp.degree;
    spl_nctrl_=sp.nControl;
    spl_nknot_=sp.nKnots;
    splp_.clear();
    splk_.clear();
  }
  else
      insight::dbg()<<"=> filtered ("<<layerpattern_<<")"<<std::endl;
}

void DXFReader::addKnot(const DL_KnotData& kd)
{
  if (notFiltered())
  {
    splk_.push_back(kd.k);
    if ((splk_.size()==spl_nknot_) && (splp_.size()==spl_nctrl_)) buildSpline();
  }
}

void DXFReader::addControlPoint(const DL_ControlPointData& cp)
{
  if (notFiltered())
  {
    splp_.push_back(gp_Pnt(cp.x, cp.y, cp.z));

    if ((splk_.size()==spl_nknot_) && (splp_.size()==spl_nctrl_))
      buildSpline();
  }
}

void DXFReader::buildSpline()
{

  int np=splp_.size(), nk=splk_.size();
  TColgp_Array1OfPnt poles(1, np);
  TColStd_Array1OfReal weights(1, np);
  int i;
  for(i=0;i<np;++i)
  {
    poles.SetValue(i+1, splp_[i]);
  }
  for(i=0;i<np;++i)
  {
    weights.SetValue(i+1, 1);
  }

  int degree = spl_deg_;
  int nkr=spl_nknot_; //+degree+1-2*degree;

  std::vector<int> mult;
  std::vector<double> u;
  u.push_back(splk_[0]);
  mult.push_back(1);
  for (int i=1; i<splk_.size(); i++)
  {
    double cu=splk_[i];
    double lu=splk_[i-1];
    if (fabs(cu-lu)<1e-10)
    {
      mult.back()++;
    }
    else
    {
      u.push_back(cu);
      mult.push_back(1);
    }
  }

  TColStd_Array1OfReal UKnots(1, u.size());
  TColStd_Array1OfInteger UMult(1, mult.size());
  for(i=0;i<u.size();++i)
  {
    UKnots.SetValue(i+1, u[i]);
  }
  for(i=0;i<mult.size();++i)
  {
    UMult.SetValue(i+1, mult[i]);
  }

  Handle_Geom_BSplineCurve c = new Geom_BSplineCurve
      (
          poles,
          weights,
          UKnots,
          UMult,
          degree
      );


    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c).Edge();
    ls_[curLayerName()].Append(e);
}



Handle_TopTools_HSequenceOfShape DXFReader::Wires(double tol, const std::string& layername) const
{
  pl_.reset(); // Finalize

  Handle(TopTools_HSequenceOfShape) inEdges(new TopTools_HSequenceOfShape);

  auto useEdgesFrom = [&](const TopTools_ListOfShape& ls)
  {
      for (auto i=ls.begin(); i!=ls.end(); ++i)
      {
          inEdges->Append(*i);
      }
  };

  for (const auto& l: ls_)
  {
      if (layername.empty())
      {
          useEdgesFrom(l.second);
      }
      else
      {
          if (l.first==layername)
          {
              useEdgesFrom(l.second);
          }
      }
  }

//  std::map<std::string, TopTools_ListOfShape>::const_iterator ld;
//  if (layername!="")
//    {
//      ld=ls_.find(layername_);
//    }
//  else
//    {
//      ld=ls_.find(layername);
//    }

  if (inEdges->Size()==0)
    throw insight::Exception("Could not retrieve data for layer \""+layername+"\"!");

  ShapeFix_ShapeTolerance sft;
  for (
    auto li=inEdges->begin();
    li!=inEdges->end(); ++li
  )
  {
    sft.SetTolerance(const_cast<TopoDS_Shape&>(*li), tol, TopAbs_VERTEX );
  }

  Handle(TopTools_HSequenceOfShape) outWires(new TopTools_HSequenceOfShape);

  ShapeAnalysis_FreeBounds::ConnectEdgesToWires(inEdges, tol, false, outWires);
//  BRepBuilderAPI_MakeWire wb;
//  wb.Add(ld->second);
//  return wb.Wire();

  return outWires;
}

std::vector<std::string> DXFReader::layers(const std::string& pattern) const
{
    return layers(boost::regex(pattern));
}

std::vector<std::string> DXFReader::layers(const boost::regex &re) const
{
    std::vector<std::string> ll;
    for (const auto& l: ls_)
    {
        if (boost::regex_match(l.first, re))
            ll.push_back(l.first);
    }
    return ll;
}


}
}
