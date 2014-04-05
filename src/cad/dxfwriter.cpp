/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
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
#include "boost/lexical_cast.hpp"

#include "base/exception.h"

using namespace std;
using namespace boost;
using namespace boost::assign;

namespace insight {
namespace cad {
  
DXFWriter::DXFWriter
(
  const boost::filesystem::path& file,
  const std::vector<std::string>& layers
)
: exportVersion_(DL_Codes::AC1015),
  dw_( dxf_.out(file.c_str(), exportVersion_) )
{
  dxf_.writeHeader(*dw_);
  dw_->sectionEnd();

  dw_->sectionTables();
  dxf_.writeVPort(*dw_);

  dw_->tableLineTypes(25);
  dxf_.writeLineType(*dw_, DL_LineTypeData("BYBLOCK", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("BYLAYER", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("CONTINUOUS", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("ACAD_ISO02W100", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("ACAD_ISO03W100", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("ACAD_ISO04W100", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("ACAD_ISO05W100", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("BORDER", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("BORDER2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("BORDERX2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("CENTER", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("CENTER2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("CENTERX2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DASHDOT", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DASHDOT2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DASHDOTX2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DASHED", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DASHED2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DASHEDX2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DIVIDE", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DIVIDE2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DIVIDEX2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DOT", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DOT2", 0));
  dxf_.writeLineType(*dw_, DL_LineTypeData("DOTX2", 0));
  dw_->tableEnd();

  int numberOfLayers = 2*(1+layers.size()); // add an extra layer in addition to each defined layer for hidden lines (name=layername+"_HL")
  dw_->tableLayers(numberOfLayers);
  dxf_.writeLayer(*dw_, DL_LayerData("0", 0), DL_Attributes(std::string(""), DL_Codes::black, 30, "CONTINUOUS") );
  dxf_.writeLayer(*dw_, DL_LayerData("0_HL", 0), DL_Attributes(std::string(""), DL_Codes::l_gray, 30, "DOT2") );
  BOOST_FOREACH(const string& ln, layers)
  {
    dxf_.writeLayer(*dw_, DL_LayerData(ln, 0), DL_Attributes(std::string(""), DL_Codes::black, 30, "CONTINUOUS") );
    dxf_.writeLayer(*dw_, DL_LayerData(ln+"_HL", 0), DL_Attributes(std::string(""), DL_Codes::l_gray, 30, "DOT2") );
  }
  dw_->tableEnd();

  dxf_.writeStyle(*dw_);
  dxf_.writeView(*dw_);
  dxf_.writeUcs(*dw_);
  dw_->tableAppid(1);
  dw_->tableAppidEntry(0x12);
  dw_->dxfString(2, "InsightCAD");
  dw_->dxfInt(70, 0);
  dw_->tableEnd();

  dxf_.writeBlockRecord(*dw_);
  dw_->tableEnd();
  dw_->sectionEnd();

  dw_->sectionEntities();

}

DXFWriter::~DXFWriter()
{
  dw_->sectionEnd();

  dxf_.writeObjects(*dw_);
  dxf_.writeObjectsEnd(*dw_);
  dw_->dxfEOF();
  dw_->close();
}



void DXFWriter::writeLine(const BRepAdaptor_Curve& c, const std::string& layer)
{
  gp_Pnt p0=c.Value(c.FirstParameter());
  gp_Pnt p1=c.Value(c.LastParameter());
  cout <<p0.X()<<" "<<p0.Y()<<" >>> "<<p1.X()<<" "<<p1.Y()<<endl;

  dxf_.writeLine
  (
    *dw_,
    DL_LineData
    (
      p0.X(), p0.Y(), 0,
      p1.X(), p1.Y(), 0
    ),
    DL_Attributes(layer, 256, -1, "BYLAYER")
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
	DL_Attributes(layer, 256, -1, "BYLAYER")
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
	  DL_Attributes(layer, 256, -1, "BYLAYER")	  
	);
    }
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
    double a = v3.DotCross(v1,v2);

    // a full ellipse
   /* if (s.SquareDistance(e) < 0.001) {
        out << "<ellipse cx =\"" << p.X() << "\" cy =\"" 
            << p.Y() << "\" rx =\"" << r1 << "\"  ry =\"" << r2 << "\"/>";
    }
    // arc of ellipse
    else {
        // See also https://developer.mozilla.org/en/SVG/Tutorial/Paths
        gp_Dir xaxis = ellp.XAxis().Direction();
        Standard_Real angle = xaxis.Angle(gp_Dir(1,0,0));
        angle = Base::toDegrees<double>(angle);
        char las = (l-f > D_PI) ? '1' : '0'; // large-arc-flag
        char swp = (a < 0) ? '1' : '0'; // sweep-flag, i.e. clockwise (0) or counter-clockwise (1)
        out << "<path d=\"M" << s.X() <<  " " << s.Y()
            << " A" << r1 << " " << r2 << " "
            << angle << " " << las << " " << swp << " "
            << e.X() << " " << e.Y() << "\" />";
    }*/
        gp_Dir xaxis = ellp.XAxis().Direction();
        double angle = xaxis.Angle(gp_Dir(1,0,0));
        //double rotation = Base::toDegrees<double>(angle);

	double ax = s.X() - p.X();
	double ay = s.Y() - p.Y();
	double bx = e.X() - p.X();
	double by = e.Y() - p.Y();

	double start_angle = atan2(ay, ax) * 180./M_PI;
	double end_angle = atan2(by, bx) * 180./M_PI;

	double major_x = r1 * sin(angle*90.);
	double major_y = r1 * cos(angle*90.);

	double ratio = r2/r1;

	if (a>0) std::swap(start_angle, end_angle);
	
	dxf_.writeEllipse
	(
	  *dw_,
	  DL_EllipseData
	  (
	    p.X(), p.Y(), 0,
	    major_x, major_y, 0,
	    ratio, start_angle, end_angle
	  ),
	  DL_Attributes(layer, 256, -1, "BYLAYER")
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
	
	cout<<"Processing curve of type "<<adapt.GetType()<<endl;
	
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

	  case GeomAbs_Ellipse:
	  {
	    writeEllipse(adapt, layer);
	  } break;

// 	  default:
// 	    throw insight::Exception("DXFWriter::writeShapeEdges : Unsupported curve type: "+lexical_cast<string>(adapt.GetType()));
	}
    }
}

}
}