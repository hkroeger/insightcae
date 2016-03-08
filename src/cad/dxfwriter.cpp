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

#include "dxfwriter.h"

#include "boost/foreach.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/lexical_cast.hpp"

#include "base/exception.h"

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight {
namespace cad {
  
std::vector<gp_Pnt> discretizeBSpline(const BRepAdaptor_Curve& c)
{
  GeomAdaptor_Curve ac(c.Curve().Curve(), c.FirstParameter(), c.LastParameter());
  std::vector<gp_Pnt> res;
  
  double deflection=0.01;
  GCPnts_UniformDeflection discretizer;
  discretizer.Initialize (ac, deflection);
  if (!discretizer.IsDone())
    throw insight::Exception("Discretization of curve failed!");
  if (discretizer.NbPoints () < 2)
    throw insight::Exception("Discretization of curve yielded less than 2 points!");
  
  gp_Pnt p0=c.Value(c.FirstParameter());
  gp_Pnt p1=c.Value(c.LastParameter());
  
  res.push_back(p0);
  for (int i=2; i<=discretizer.NbPoints()-1; i++)
  {
    res.push_back(c.Value(discretizer.Parameter(i)));
  }
  res.push_back(p1);
  
  
//   cout<<p0.X()<<" "<<p0.Y()<<" "<<p0.Z()<<" <=> "<<res[0].X()<<" "<<res[0].Y()<<" "<<res[0].Z()<<endl;
//   cout<<p1.X()<<" "<<p1.Y()<<" "<<p1.Z()<<" <=> "<<res.back().X()<<" "<<res.back().Y()<<" "<<res.back().Z()<<endl;

  return res;
}


DXFWriter::DXFWriter
(
  const boost::filesystem::path& file,
  const std::vector<LayerDefinition>& layers
)
: exportVersion_(DL_Codes::AC1015),
  dw_( dxf_.out(file.c_str(), exportVersion_) )
{
  dxf_.writeHeader(*dw_);
  dw_->sectionEnd();

  dw_->sectionTables();
  dxf_.writeVPort(*dw_);

  dw_->tableLinetypes(25);
  dxf_.writeLinetype(*dw_, DL_LinetypeData("BYBLOCK", "", 0, 1, 1));
  dxf_.writeLinetype(*dw_, DL_LinetypeData("BYLAYER", "", 0, 1, 1));
  dxf_.writeLinetype(*dw_, DL_LinetypeData("CONTINUOUS", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("ACAD_ISO02W100", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("ACAD_ISO03W100", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("ACAD_ISO04W100", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("ACAD_ISO05W100", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("BORDER", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("BORDER2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("BORDERX2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("CENTER", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("CENTER2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("CENTERX2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DASHDOT", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DASHDOT2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DASHDOTX2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DASHED", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DASHED2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DASHEDX2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DIVIDE", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DIVIDE2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DIVIDEX2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DOT", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DOT2", "", 0, 1, 1));
//   dxf_.writeLinetype(*dw_, DL_LinetypeData("DOTX2", "", 0, 1, 1));
  dw_->tableEnd();

  int numberOfLayers = 3+2*(layers.size()); // add an extra layer in addition to each defined layer for hidden lines (name=layername+"_HL")
  dw_->tableLayers(numberOfLayers);
  dxf_.writeLayer(*dw_, DL_LayerData("ANNOTATIONS", 0), DL_Attributes(std::string(""), DL_Codes::black, 35, "CONTINUOUS", 1) );
  dxf_.writeLayer(*dw_, DL_LayerData("0", 0), DL_Attributes(std::string(""), DL_Codes::black, 50, "CONTINUOUS", 1) );
  dxf_.writeLayer(*dw_, DL_LayerData("0_HL", 0), DL_Attributes(std::string(""), DL_Codes::l_gray, 35, "CONTINUOUS", 1) );
  BOOST_FOREACH(const LayerDefinition& ld, layers)
  {
    DL_LayerData ldata(boost::get<0>(ld), 0);
    DL_Attributes lattr=boost::get<1>(ld);
    dxf_.writeLayer(*dw_, ldata, lattr );
    
    if (boost::get<2>(ld))
    {
      ldata.name+="_HL";
      lattr.setColor(DL_Codes::l_gray);
      lattr.setWidth(35);
      dxf_.writeLayer(*dw_, ldata, lattr );
    }
  }
  dw_->tableEnd();

#warning needs handling!
//   dxf_.writeStyle(*dw_);
  dxf_.writeView(*dw_);
  dxf_.writeUcs(*dw_);
  dw_->tableAppid(1);
  dw_->tableAppidEntry(0x12);
  dw_->dxfString(2, "InsightCAD");
  dw_->dxfInt(70, 0);
  dw_->tableEnd();

  dxf_.writeBlockRecord(*dw_);
  dw_->sectionEnd();
}

DXFWriter::~DXFWriter()
{
  dxf_.writeObjects(*dw_);
  dxf_.writeObjectsEnd(*dw_);
  dw_->dxfEOF();
  dw_->close();
}



void DXFWriter::writeLine(const BRepAdaptor_Curve& c, const std::string& layer)
{
  gp_Pnt p0=c.Value(c.FirstParameter());
  gp_Pnt p1=c.Value(c.LastParameter());

  dxf_.writeLine
  (
    *dw_,
    DL_LineData
    (
      p0.X(), p0.Y(), 0,
      p1.X(), p1.Y(), 0
    ),
    DL_Attributes(layer, 256, -1, "BYLAYER", 1)
  );
}

writerLine_HatchLoop::writerLine_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer, bool reverse)
{
  p0=c.Value(c.FirstParameter());
  p1=c.Value(c.LastParameter());
  
  if (reverse)
  {
    std::swap(p0, p1);
    //cout<<"curve reversed"<<endl;
  }
}

void writerLine_HatchLoop::write(DL_Dxf& dxf, std::auto_ptr<DL_WriterA>& dw) const
{
//   {
//     std::ofstream f("debug.txt", fstream::app);
//     f<<endl<<endl;
//     f<<p0.X()<<" "<<p0.Y()<<endl;
//     f<<p1.X()<<" "<<p1.Y()<<endl;
//   }
  
  dxf.writeHatchEdge
  (
    *dw,
    DL_HatchEdgeData
    (
      p0.X(), p0.Y(),
      p1.X(), p1.Y()
    )
  );
}

void DXFWriter::writeCircle(const BRepAdaptor_Curve& c, const std::string& layer)
{
    gp_Circ circ = c.Circle();
	//const gp_Ax1& axis = c->Axis();
    const gp_Pnt& p= circ.Location();
    double r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1.);
    double a = v3.DotCross(v1,v2);

    // a full circle
    if (s.SquareDistance(e) < 0.001) 
    {
      dxf_.writeCircle(
	*dw_,
	DL_CircleData
	(
	  p.X(), p.Y(), 0,
	 r
	),
	DL_Attributes(layer, 256, -1, "BYLAYER", 1)
      );
    }

    // arc of circle
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        /*char xar = '0'; // x-axis-rotation
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r << " " << r << " "
            << xar << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";*/
	double ax = s.X() - p.X();
	double ay = s.Y() - p.Y();
	double bx = e.X() - p.X();
	double by = e.Y() - p.Y();

	double start_angle = atan2(ay, ax) * 180./M_PI;
	double end_angle = atan2(by, bx) * 180./M_PI;


	if(a > 0)
	{
		double temp = start_angle;
		start_angle = end_angle;
		end_angle = temp;
	}
	dxf_.writeArc
	(
	  *dw_,
	  DL_ArcData
	  (
	    p.X(), p.Y(), 0,
	    r, start_angle, end_angle
	  ),
	  DL_Attributes(layer, 256, -1, "BYLAYER", 1)	  
	);
    }
}

writerCircle_HatchLoop::writerCircle_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer)
{
    gp_Circ circ = c.Circle();
	//const gp_Ax1& axis = c->Axis();
    p= circ.Location();
    r = circ.Radius();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m,s);
    gp_Vec v2(m,e);
    gp_Vec v3(0,0,1.);
    double a = v3.DotCross(v1,v2);

    // a full circle
    if (s.SquareDistance(e) < 0.001) 
    {
      start_angle=0.0;
      end_angle=2.*M_PI;
    }

    // arc of circle
    else {

	double ax = s.X() - p.X();
	double ay = s.Y() - p.Y();
	double bx = e.X() - p.X();
	double by = e.Y() - p.Y();

	start_angle = atan2(ay, ax);
	end_angle = atan2(by, bx);


	if (a > 0) std::swap(start_angle, end_angle);
    }
}

void writerCircle_HatchLoop::write(DL_Dxf& dxf, std::auto_ptr<DL_WriterA>& dw) const
{
  dxf.writeHatchEdge
  (
    *dw,
    DL_HatchEdgeData
    (
      p.X(), p.Y(),
      r,
      start_angle, end_angle,
      true
    )
  );
}

void DXFWriter::writeDiscrete(const BRepAdaptor_Curve& c, const std::string& layer)
{
//   writeLine(c, layer);
  std::vector<gp_Pnt> pts=discretizeBSpline(c);

  dxf_.writePolyline
  (
        *dw_,
        DL_PolylineData(
                 pts.size(), 
                 0, 0, 
                 false
        ),
        DL_Attributes(layer, 256, -1, "BYLAYER", 1.)
  );

//   for (int i=1; i<pts.size(); i++)
  for (int i=0; i<pts.size(); i++)
  {
//     gp_Pnt p0=pts[i-1];
    gp_Pnt p1=pts[i];
    
//     dxf_.writeLine
//     (
//       *dw_,
//       DL_LineData
//       (
// 	p0.X(), p0.Y(), 0,
// 	p1.X(), p1.Y(), 0
//       ),
//       DL_Attributes(layer, 256, -1, "BYLAYER", 1.)
//     );
    // for every vertex in the polyline:
    dxf_.writeVertex
    (
	  *dw_,
	  DL_VertexData(
		p1.X(), p1.Y(), 0, 
		0.0
	  )
    );
  }

  dxf_.writePolylineEnd(*dw_);

}
  
writerDiscrete_HatchLoop::writerDiscrete_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer, bool reverse)
{
  pts=discretizeBSpline(c);
  
  // Replace first and last point from Geom_Curve discretization by Topological end vertices
  // => consecutive segments thus have identically the same end/start coordinates
  pts[0]=BRep_Tool::Pnt(TopExp::FirstVertex(c.Edge()));
  pts.back()=BRep_Tool::Pnt(TopExp::LastVertex(c.Edge()));
  
  if (reverse)
  {
    std::reverse(pts.begin(), pts.end());
    //cout<<"curve reversed"<<endl;
  }
  

}

void writerDiscrete_HatchLoop::write(DL_Dxf& dxf, auto_ptr< DL_WriterA >& dw) const
{
//   {
//     std::ofstream f("debug.txt", fstream::app);
//     f<<endl<<endl;
//     for (int i=0; i<pts.size(); i++)
//     {
//       f<<pts[i].X()<<" "<<pts[i].Y()<<endl;
//     }
//   }
//   
  for (int i=1; i<pts.size(); i++)
  {
    gp_Pnt p0=pts[i-1];
    gp_Pnt p1=pts[i];
    
    dxf.writeHatchEdge
    (
      *dw,
      DL_HatchEdgeData
      (
	p0.X(), p0.Y(),
	p1.X(), p1.Y()
      )
    );
  }  
}

HatchGenerator::HatchData::HatchData(double s, double a)
: scale(s), angle(a)
{
}

std::vector<HatchGenerator::HatchData> HatchGenerator::hatches_ =
 list_of<HatchGenerator::HatchData>
  (HatchData(0.33, 0.0))
  (HatchData(0.33, -90.0))
  (HatchData(0.33, 10.0))
  (HatchData(0.33, -80.0))
  (HatchData(0.33, 20.0))
  (HatchData(0.33, -70.0))
  (HatchData(0.5, 0.0))
  (HatchData(0.5, -90.0))
  (HatchData(0.5, 10.0))
  (HatchData(0.5, -80.0))
  (HatchData(0.5, 20.0))
  (HatchData(0.5, -70.0))
  (HatchData(0.66, 0.0))
  (HatchData(0.66, -90.0))
  (HatchData(0.66, 10.0))
  (HatchData(0.66, -80.0))
  (HatchData(0.66, 20.0))
  (HatchData(0.66, -70.0))
 ;

HatchGenerator::HatchGenerator()
: curidx_(0)
{}

DL_HatchData HatchGenerator::generate()
{
  const HatchData& hd=hatches_[curidx_];
  curidx_++;
  if (curidx_>=hatches_.size()) curidx_=0;
  return DL_HatchData(1, false, hd.scale, hd.angle, /*"iso03w100"*/ "ANSI31" );
}
  
void DXFWriter::writeEllipse(const BRepAdaptor_Curve& c, const std::string& layer)
{
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt& p= ellp.Location();
    
    double r1 = ellp.MajorRadius();
    double r2 = ellp.MinorRadius();
    
    double f = c.FirstParameter();
    double l = c.LastParameter();
    
    gp_Pnt s = c.Value(f);
    gp_Pnt e = c.Value(l);
    gp_Pnt m = c.Value(0.5*(l+f));

    gp_Vec v1(m, s);
    gp_Vec v2(m, e);
    gp_Vec v3(0, 0, 1.);
    double dir = v3.DotCross(v1,v2);

    gp_Dir xaxis = ellp.XAxis().Direction();
    gp_Pnt pm=gp_Pnt(xaxis.XYZ().Normalized()*r1);

    gp_Vec a (p, s); // = s-p
    gp_Vec b (p, e); // = e-p

    double a0=atan2(xaxis.Y(), xaxis.X());
    if (a0<0) a0=2.*M_PI-a0;
    a.Rotate(gp_Ax1(p, v3), -a0);
    b.Rotate(gp_Ax1(p, v3), -a0);
    double start_angle = atan2(a.Y(), a.X());
    if (start_angle<0) start_angle=2.*M_PI-start_angle;
    double end_angle = atan2(b.Y(), b.X());
    if (end_angle<0) end_angle=2.*M_PI-end_angle;

    double ratio = r2/r1;

    if (dir>0) std::swap(start_angle, end_angle);
    
  dxf_.writeLine
  (
    *dw_,
    DL_LineData
    (
      p.X(), p.Y(), 0,
      s.X(), s.Y(), 0
    ),
    DL_Attributes("ANNOTATIONS", 256, -1, "BYLAYER", 1)
  );
  dxf_.writeLine
  (
    *dw_,
    DL_LineData
    (
      p.X(), p.Y(), 0,
      e.X(), e.Y(), 0
    ),
    DL_Attributes("ANNOTATIONS", 256, -1, "BYLAYER", 1)
  );
  dxf_.writeLine
  (
    *dw_,
    DL_LineData
    (
      p.X(), p.Y(), 0,
      m.X(), m.Y(), 0
    ),
    DL_Attributes("ANNOTATIONS", 256, -1, "BYLAYER", 1)
  );
  dxf_.writeLine
  (
    *dw_,
    DL_LineData
    (
      p.X(), p.Y(), 0,
      pm.X(), pm.Y(), 0
    ),
    DL_Attributes("ANNOTATIONS", 256, -1, "BYLAYER", 1)
  );

  dxf_.writeEllipse
    (
      *dw_,
      DL_EllipseData
      (
	p.X(), p.Y(), 0,
	pm.X(), pm.Y(), 0,
	ratio, 
	start_angle, end_angle
      ),
      DL_Attributes(layer, 256, -1, "BYLAYER", 1)
    );
}

void DXFWriter::writeShapeEdges(const TopoDS_Shape& shape, std::string layer)
{
    TopExp_Explorer ex;
    for (ex.Init(shape, TopAbs_EDGE); ex.More(); ex.Next())
    {
	TopoDS_Edge e=TopoDS::Edge(ex.Current());
	BRepLib::BuildCurve3d(e);
	
	BRepAdaptor_Curve adapt(e);
	
	//cout<<"Processing curve of type "<<adapt.GetType()<<endl;
	
	switch (adapt.GetType())
	{
	  case GeomAbs_Line:
	  {
	    writeLine(adapt, layer);
	  } break;
	    
	  case GeomAbs_Circle:
	  {
	    writeCircle(adapt, layer);
	  } break;

// 	  case GeomAbs_Ellipse:
// 	  {
// 	    writeEllipse(adapt, layer);
// 	  } break;

	  default:
	    writeDiscrete(adapt, layer);
	}
    }
}

void DXFWriter::writeSection(const TopoDS_Shape& shape, HatchGenerator& hgen, std::string layer)
{
  for (TopExp_Explorer exf(shape, TopAbs_FACE); exf.More(); exf.Next())
  {
    //cout<<"Processing face"<<endl;
    
    TopoDS_Face f=TopoDS::Face(exf.Current());
    
    boost::ptr_vector<hatchLoopWriter> segments;
    for (TopExp_Explorer exw(f, TopAbs_WIRE); exw.More(); exw.Next())
    {
      //cout << "ADDING WIRE" << endl;
      // loop through edges of wire in ordered manner
      for (BRepTools_WireExplorer ex(TopoDS::Wire(exw.Current())); ex.More(); ex.Next())
      {
	  TopoDS_Edge e=ex.Current();
	  BRepLib::BuildCurve3d(e);
	  
	  BRepAdaptor_Curve adapt(e);
	  
	  cout<<"xsec drawing: processing curve of type "<<adapt.GetType()<<endl;
	  
	  switch (adapt.GetType())
	  {
	    case GeomAbs_Line:
	    {
	      segments.push_back(new writerLine_HatchLoop(adapt, layer, ex.Orientation()==TopAbs_REVERSED));
	    } break;
	      
	    case GeomAbs_Circle:
  // 	  {
  // 	    writeCircle_HatchLoop(adapt, layer);
  // 	  } break;

	    case GeomAbs_Ellipse:
	    case GeomAbs_Hyperbola:
	    case GeomAbs_Parabola:
	    case GeomAbs_BSplineCurve:
	    case GeomAbs_BezierCurve:
	    {
	      segments.push_back(new writerDiscrete_HatchLoop(adapt, layer, ex.Orientation()==TopAbs_REVERSED));
	    } break;

	    default:
	      segments.push_back(new writerLine_HatchLoop(adapt, layer, ex.Orientation()==TopAbs_REVERSED));
	  }
      }
    }
    
    DL_Attributes attributes(layer, 256, 0, -1, "BYLAYER");
 
    // start hatch with one loop:
    DL_HatchData data = hgen.generate(); //(1, false, 0.5, 45.0, "iso03w100");
 
    // start loop:
    int nsegs=0;
    BOOST_FOREACH(const hatchLoopWriter& w, segments)
    {
      nsegs+=w.nsegments();
    }
    //cout<<"nsegs="<<nsegs<<endl;
    DL_HatchLoopData lData(nsegs);
    
    // start hatch with one loop:
    dxf_.writeHatch1(*dw_, data, attributes);
 
    // start loop:
    dxf_.writeHatchLoop1(*dw_, lData);
    
    for (int i=0; i<segments.size(); i++)
    {
      hatchLoopWriter& w=segments[i];
      if (i>0)
      {
	gp_Pnt lpe=segments[i-1].end();
	w.alignStartWith(lpe);
	gp_Pnt ps=w.start();
	//cout<<lpe.X()<<" "<<lpe.Y()<<" "<<lpe.Z()<<" <=> "<<ps.X()<<" "<<ps.Y()<<" "<<ps.Z()<<" >> "<<lpe.Distance(ps)<<endl;
      }
      else
      {
	gp_Pnt lpe=segments.back().end();
	w.alignStartWith(lpe);
	gp_Pnt ps=w.start();
	//cout<<lpe.X()<<" "<<lpe.Y()<<" "<<lpe.Z()<<" <=> "<<ps.X()<<" "<<ps.Y()<<" "<<ps.Z()<<" >> "<<lpe.Distance(ps)<<endl;
      }
      w.write(dxf_, dw_);
    }  
    
    // end loop:
    dxf_.writeHatchLoop2(*dw_, lData);
 
    // end hatch:
    dxf_.writeHatch2(*dw_, data, attributes);
  }
}

void DXFWriter::writeViews(const boost::filesystem::path& file, const Feature::Views& views)
{
  std::vector<LayerDefinition> addlayers;
  
  BOOST_FOREACH(const Feature::Views::value_type& v, views)
  {
    string name=v.first;
    addlayers.push_back
    (
      LayerDefinition(name, DL_Attributes(std::string(""), DL_Codes::black, 50, "CONTINUOUS", 1.), true)
    );
    if (!v.second.crossSection.IsNull())
    {
      addlayers.push_back
      (
	LayerDefinition(name+"_XSEC", DL_Attributes(std::string(""), DL_Codes::black, 25, "CONTINUOUS", 1.), false)
      );
    }
  }

  DXFWriter dxf(file, addlayers);

//   dw_->sectionEntities();

  dxf.dw().sectionBlocks();
  HatchGenerator hgen;
  BOOST_FOREACH(const Feature::Views::value_type& v, views)
  {
    string name=v.first;
    
    dxf.dxf().writeBlock(dxf.dw(),
      DL_BlockData(name, 0, 0.0, 0.0, 0.0));
    dxf.writeShapeEdges(v.second.visibleEdges, name);
    dxf.writeShapeEdges(v.second.hiddenEdges, name+"_HL");
    if (!v.second.crossSection.IsNull())
    {
      dxf.writeSection( v.second.crossSection, hgen, name+"_XSEC");
    }
    dxf.dxf().writeEndBlock(dxf.dw(), name);
  }
  dxf.dw().sectionEnd();
  
  dxf.dw().sectionEntities();
//   dxf.dxf().writeBlock();
  dxf.dw().sectionEnd();
}
  
}
}