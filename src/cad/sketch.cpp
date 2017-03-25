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

#include "datum.h"

#include "TColStd_Array1OfInteger.hxx"
#include "GC_MakeArcOfCircle.hxx"

#include <chrono>

using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


namespace insight {
namespace cad {

DXFReader::DXFReader(const boost::filesystem::path& filename, const std::string& layername)
: layername_(layername)
{
  std::auto_ptr<DL_Dxf> dxf(new DL_Dxf());
  if (!dxf->in(filename.c_str(), this)) 
  {
    std::cerr << filename << " could not be opened." << std::endl;
  }
}

DXFReader::~DXFReader()
{
}

void DXFReader::addArc(const DL_ArcData &a)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    gp_Pnt cp(a.cx, a.cy, a.cz);
    gp_Circ c = gce_MakeCirc( gp_Ax2(cp, gp_Dir(0,0,1)), a.radius );
    gp_Pnt p0(cp); p0.Translate(gp_Vec(a.radius*::cos(M_PI*a.angle1/180.), a.radius*::sin(M_PI*a.angle1/180.), 0) );
    gp_Pnt p1(cp); p1.Translate(gp_Vec(a.radius*::cos(M_PI*a.angle2/180.), a.radius*::sin(M_PI*a.angle2/180.), 0) );
//     Standard_Real Alpha1 = ElCLib::Parameter(c, p0);
//     Standard_Real Alpha2 = ElCLib::Parameter(c, p1);
//     Handle(Geom_Circle) C = new Geom_Circle(c);
//     Handle(Geom_TrimmedCurve) arc = new Geom_TrimmedCurve(C, Alpha1, Alpha2, false);
    cout<<a.cx<<" "<<a.cy<<" "<<a.cz<<endl;
    cout<<a.radius<<" "<<a.angle1<<" "<<a.angle2<<endl;
    cout<<p0.X()<<" "<<p0.Y()<<" "<<p0.Z()<<endl;
    cout<<p1.X()<<" "<<p1.Y()<<" "<<p1.Z()<<endl;
    cout<<"added arc"<<endl;
//     TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c, BRepBuilderAPI_MakeVertex(p0), BRepBuilderAPI_MakeVertex(p1)).Edge();
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c, p0, p1).Edge();
    ls_.Append(e);
  }
}

void DXFReader::addLine (const DL_LineData &l)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    gp_Pnt p0(l.x1, l.y1, l.z1);
    gp_Pnt p1(l.x2, l.y2, l.z2);
    cout<<"added line"<<endl;
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    ls_.Append(e);
  }
}

void DXFReader::addPolyline(const DL_PolylineData &pl)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    pl_.reset(new Polyline(*this));
    pl_->closed=false;
    
    pl_->lp.reset();
    
    if (pl.flags & 1)
    {
      cout<<"closed polyline!"<<endl;
      pl_->p0.reset();
      pl_->closed=true;
    }
  }
}

DXFReader::Polyline::Polyline(DXFReader& r)
: reader(r), lbulge(0.0)
{
}


DXFReader::Polyline::~Polyline()
{
    if ( closed )
    {
      cout<<"added closing line "<<endl;
      DL_VertexData pv;
      pv.x=p0->X();
      pv.y=p0->Y();
      pv.z=p0->Z();
      pv.bulge=0;
      reader.addVertex(pv);
//       TopoDS_Edge e=BRepBuilderAPI_MakeEdge(*lp, *p0).Edge();
//       reader.ls_.Append(e);
    }
}



void DXFReader::addVertex(const DL_VertexData &pv)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    gp_Pnt p(pv.x, pv.y, pv.z);
    
    if (!pl_->p0.get())
    {
      pl_->p0.reset(new gp_Pnt(p));
    }
    else
    {
      double bulge=pl_->lbulge;
      if (fabs(bulge)<1e-10)
      {
	cout<<"added polyline line segment"<<endl;
	TopoDS_Edge e=BRepBuilderAPI_MakeEdge(*pl_->lp, p).Edge();
	ls_.Append(e);
      }
      else
      {
	cout<<"added polyline arc segment"<<endl;
	gp_XYZ pa(pl_->lp->XYZ());
	gp_XYZ pb(p.XYZ());
	double u=(pb-pa).Modulus();
// 	double r=u*(b*b+1.0)/4.0/b;
	double i=bulge*u/2.0;
// 	double a=r-i;
	gp_XYZ er=(pb-pa).Crossed(gp_XYZ(0,0,1)).Normalized();
	gp_XYZ pt( pa + 0.5*(pb-pa) + i*er );
	TopoDS_Edge e=BRepBuilderAPI_MakeEdge(
	  GC_MakeArcOfCircle(gp_Pnt(pa), gp_Pnt(pt), gp_Pnt(pb)), 
	  gp_Pnt(pa), gp_Pnt(pb)
	).Edge();
	std::cout<<"bulge="<<bulge<<" i="<<i<<std::endl
	 <<"["<<pa.X()<<" "<<pa.Y()<<" "<<pa.Z()<<"]"<<std::endl
	 <<"["<<pb.X()<<" "<<pb.Y()<<" "<<pb.Z()<<"]"<<std::endl
	 <<"["<<pt.X()<<" "<<pt.Y()<<" "<<pt.Z()<<"]"<<std::endl;
// 	TopoDS_Edge e=BRepBuilderAPI_MakeEdge(*pl_->lp, p).Edge();
	ls_.Append(e);
      }
    }
    pl_->lp.reset(new gp_Pnt(p));
    pl_->lbulge=pv.bulge;
  }
}


void DXFReader::addSpline(const DL_SplineData& sp)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    cout<<"addSpline"<<endl;
    spl_deg_=sp.degree;
    spl_nctrl_=sp.nControl;
    spl_nknot_=sp.nKnots;
    splp_.clear();
    splk_.clear();
  }
}

void DXFReader::addKnot(const DL_KnotData& kd)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    cout<<"addknot"<<endl;
    splk_.push_back(kd.k);
    if ((splk_.size()==spl_nknot_) && (splp_.size()==spl_nctrl_)) buildSpline();
  }
}

void DXFReader::addControlPoint(const DL_ControlPointData& cp)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    cout<<"addctrlp"<<endl;
    splp_.push_back(gp_Pnt(cp.x, cp.y, cp.z));
    if ((splk_.size()==spl_nknot_) && (splp_.size()==spl_nctrl_)) buildSpline();
  }
}

void DXFReader::buildSpline()
{
  cout<<"building spline"<<endl;

  int np=splp_.size(), nk=splk_.size();
  TColgp_Array1OfPnt poles(1, np);
  TColStd_Array1OfReal weights(1, np);
  int i;
  for(i=0;i<np;++i)
  {
    cout<<i<<" : "<<splp_[i].X()<<" "<<splp_[i].Y()<<" "<<splp_[i].Z()<<endl;
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
    cout<<i<<" : "<<u[i]<<endl;
    UKnots.SetValue(i+1, u[i]);
  }
  for(i=0;i<mult.size();++i)
  {
    cout<<i<<" : "<<mult[i]<<endl;
    UMult.SetValue(i+1, mult[i]);
  }
//   UMult.SetValue(1,degree+1);
//   UMult.SetValue(nkr,degree+1);

  Handle_Geom_BSplineCurve c = new Geom_BSplineCurve
      (
          poles,
          weights,
          UKnots,
          UMult,
          degree
      );

      
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c).Edge();
    ls_.Append(e);      
}



TopoDS_Wire DXFReader::Wire(double tol) const
{
  pl_.reset(); // Finalize
  
  ShapeFix_ShapeTolerance sft;
  for (
    TopTools_ListIteratorOfListOfShape li(ls_);
    li.More(); li.Next()
  )
  {
    sft.SetTolerance(const_cast<TopoDS_Shape&>(li.Value()), tol, TopAbs_VERTEX );
  }
  
  BRepBuilderAPI_MakeWire wb;
  wb.Add(ls_);
  return wb.Wire();
}

// TopoDS_Shape Sketch::makeSketch(const Datum& pl, const boost::filesystem::path& fn, const std::string& ln)
// {
// }

defineType(Sketch);
addToFactoryTable(Feature, Sketch);

Sketch::Sketch()
: Feature()
{}


Sketch::Sketch
(
  DatumPtr pl, 
  const boost::filesystem::path& fn, 
  const std::string& ln, 
  const SketchVarList& vars,
  double tol
)
: pl_(pl),
  fn_(fn),
  ln_(ln),
  vars_(vars),
  tol_(tol)
{
  ParameterListHash p(this);
  p+=*pl_;
  try 
  {
    // try to incorporate file time stamp etc
    p+=sharedModelFilePath(fn_.string());
  }
  catch (...)
  {
    // if file is non-existing, use filename only
    p+=fn_.string();
  }
  p+=ln_;
  for (SketchVarList::const_iterator it=vars_.begin(); it!=vars_.end(); it++)
  {
    p+=boost::fusion::at_c<0>(*it);
    p+=boost::fusion::at_c<1>(*it)->value();
  }
  p+=tol_;
}





FeaturePtr Sketch::create
(
    DatumPtr pl,
    const boost::filesystem::path& fn,
    const std::string& ln,
    const SketchVarList& vars,
    double tol
)
{
    return FeaturePtr
           (
               new Sketch
               (
                   pl, fn,
                   ln, vars,
                   tol
               )
           );
}




void Sketch::build()
{
    auto t_start = std::chrono::high_resolution_clock::now();
    
    if (!cache.contains(hash()))
    {
        if (!pl_->providesPlanarReference())
            throw insight::Exception("Sketch: Planar reference required!");

        boost::filesystem::path infilename = fn_;
        if (!exists(infilename))
            infilename=sharedModelFilePath(fn_.string());

        boost::filesystem::path filename = fn_;
        std::string layername = ln_;

        std::string ext=fn_.extension().string();
        boost::algorithm::to_lower(ext);
        //   cout<<ext<<endl;

        if (ext==".fcstd")
        {
            filename =
                boost::filesystem::unique_path( temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.dxf" );
            boost::filesystem::path macrofilename =
                boost::filesystem::unique_path( temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.FCMacro" );
            layername="0";

            {

                std::string vargs="";
                for (SketchVarList::const_iterator it=vars_.begin(); it!=vars_.end(); it++)
                {
                    std::string vname=boost::fusion::at_c<0>(*it);
                    double vval=boost::fusion::at_c<1>(*it)->value();
                    if (starts_with(vname, "Constraint"))
                    {
                        try
                        {
                            int vid;
                            vname.erase(0,10);
                            vid=lexical_cast<int>(vname);
                            vargs+=str(format("  obj.setDatum(%d, %g)\n") % vid % vval );
                        }
                        catch (...)
                        {
                        }
                    }
                    else
                    {
                        vargs+=str(format("  obj.setDatum('%s', %g)\n") % vname % vval );
                    }
                }

                std::ofstream mf(macrofilename.c_str());
                mf << str( format(
                               "import FreeCAD\n"
                               "import importDXF\n"

                               "FreeCAD.open(\"%s\")\n"
                               "__objs__=[]\n"
                               "doc=FreeCAD.getDocument( \"%s\" );\n"
                               //"print dir(doc)\n"
                               "obj=None\n"
                               "for o in doc.Objects:\n"
                               " if (o.Label==\"%s\"):\n"
                               "  obj=o\n"
                               +vargs+
                               "  break\n"
                               //"print obj\n"
                               "__objs__.append(obj)\n"
                               "doc.recompute()\n"
                               "importDXF.export(__objs__, \"%s\")\n"
                               "del __objs__\n")
                           % infilename.string()
                           % infilename.filename().stem().string()
                           % ln_
                           % filename.string()
                         );
            }

            std::string cmd1 = str( format("FreeCADCmd %s") % macrofilename );
            std::string cmd2 = str( format("freecadcmd %s") % macrofilename );  // system-installed in debian..

            if ( ::system( cmd1.c_str() ) || !boost::filesystem::exists(filename) )
            {
                if ( ::system( cmd2.c_str() ) || !boost::filesystem::exists(filename) )
                {
                    throw insight::Exception("Conversion of FreeCAD file "+infilename.string()+" into DXF "+filename.string()+" failed!");
                }
            }
            boost::filesystem::remove(macrofilename);

        }
        else if (ext==".psketch")
        {
            filename=boost::filesystem::unique_path( temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.dxf" );
            layername="0";

            std::string vargs="";
            for (SketchVarList::const_iterator it=vars_.begin(); it!=vars_.end(); it++)
            {
                std::string vname=boost::fusion::at_c<0>(*it);
                double vval=boost::fusion::at_c<1>(*it)->value();
                vargs+=" -v"+vname+"="+lexical_cast<std::string>(vval);
            }

            std::string cmd = str( format("psketchercmd %s -o %s") % infilename % filename ) + vargs;
            cout<<"CMD=\""<<cmd<<"\""<<endl;
            if ( ::system( cmd.c_str() ) || !boost::filesystem::exists(filename) )
            {
                throw insight::Exception("Conversion of pSketch file "+infilename.string()+" into DXF "+filename.string()+" failed!");
            }
        }

        TopoDS_Wire w = DXFReader(filename, layername).Wire(tol_);
        providedSubshapes_["OuterWire"]=FeaturePtr(new Feature(w));

        gp_Trsf tr;
        gp_Ax3 ax=*pl_;
        tr.SetTransformation(ax);

        BRepBuilderAPI_Transform btr(w, tr.Inverted(), true);

        if (w.Closed())
            setShape(BRepBuilderAPI_MakeFace(gp_Pln(ax), TopoDS::Wire(btr.Shape())).Shape());
        else
            setShape(TopoDS::Wire(btr.Shape()));

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<Sketch>(hash()));
    }
    
    auto t_end = std::chrono::high_resolution_clock::now();

    std::cout << "sketch rebuild done in " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count()
              << " ms" << std::endl;
}




void Sketch::executeEditor()
{

    bool is_new_file=false;
    boost::filesystem::path infilename = fn_;
    std::string layername = ln_;

    std::string ext=fn_.extension().string();
    boost::algorithm::to_lower(ext);

    if (!exists(infilename))
    {
        try
        {
            infilename=sharedModelFilePath(fn_.string());
        }
        catch (...)
        {
            is_new_file=true;
        }
    }


    if (ext==".fcstd")
    {
        boost::filesystem::path macrofilename =
            boost::filesystem::unique_path( temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.FCMacro" );
        layername="0";

        {
            std::string vargs="";
            if (!is_new_file)
            {
                for (SketchVarList::const_iterator it=vars_.begin(); it!=vars_.end(); it++)
                {
                    std::string vname=boost::fusion::at_c<0>(*it);
                    double vval=boost::fusion::at_c<1>(*it)->value();
                    if (starts_with(vname, "Constraint"))
                    {
                        try
                        {
                            int vid;
                            vname.erase(0,10);
                            vid=lexical_cast<int>(vname);
                            vargs+=str(format(" obj.setDatum(%d, %g)\n") % vid % vval );
                        }
                        catch (...)
                        {
                        }
                    }
                    else
                    {
                        vargs+=str(format(" obj.setDatum('%s', %g)\n") % vname % vval );
                    }
                }
            }

            std::ofstream mf(macrofilename.c_str());
            mf << 
            "import FreeCAD, FreeCADGui\n";
            
            if (!is_new_file)
            {
                mf << 
                "FreeCAD.open(\""<<infilename.string()<<"\")\n"
                "doc=FreeCAD.getDocument( \""<<infilename.filename().stem().string()<<"\" );\n";
            } else 
            {
                mf << 
                "doc=FreeCAD.newDocument( \""<<infilename.filename().stem().string()<<"\" )\n"
                "doc.saveAs(\"" << boost::filesystem::absolute(infilename).string() << "\")\n";
            }
            
            mf << 
            "FreeCADGui.ActiveDocument=doc\n"
            "obj=None\n"
            "objname=\""+ln_+"\"\n"
            "for o in doc.Objects:\n"
            " if (o.Label==\""+ln_+"\"):\n"
            "  obj=o\n"
            "  objname=o.Name\n"
            "  break\n"
            "if not obj is None:\n"
            + vargs+
            " doc.recompute()\n"
            "else:\n"
            " doc.addObject('Sketcher::SketchObject','"+ln_+"')\n"
            "FreeCADGui.activeDocument().setEdit(objname)\n"
            ;

        }
        

        std::string cmd1 = str( format("FreeCAD %s") % macrofilename );
        std::string cmd2 = str( format("freecad %s") % macrofilename ); // system-installed freecad in debian...

        if ( ::system( cmd1.c_str() ) )
        {
            if ( ::system( cmd2.c_str() ) )
            {
                throw insight::Exception("Execution of command \""+cmd2+"\" failed!");
            }
        }
        boost::filesystem::remove(macrofilename);

    }
    else
    {
        throw insight::Exception("Not implemented: starting other editor than FreeCAD!");
    }
}

void Sketch::operator=(const Sketch& o)
{
  pl_=o.pl_;
  fn_=o.fn_;
  ln_=o.ln_;
  vars_=o.vars_;
  tol_=o.tol_;
  Feature::operator=(o);
}


Sketch::operator const TopoDS_Face& () const
{
  if (!shape().ShapeType()==TopAbs_FACE)
    throw insight::Exception("Shape is not a face: presumably, original wire was not closed");
  return TopoDS::Face(shape());
}

bool Sketch::isSingleFace() const
{
  return (shape().ShapeType()==TopAbs_FACE);
}


void Sketch::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Sketch",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_datumExpression > ',' 
	  > ruleset.r_path 
	  > ( ( ',' >> ruleset.r_string ) | qi::attr(std::string("sketch")) )
	  > ( ( ',' >> (ruleset.r_identifier > '=' > ruleset.r_scalarExpression )% ',' ) | qi::attr(SketchVarList()) )
	  > ( ( ',' >> qi::double_ ) | qi::attr(1e-3) ) > 
      ')' ) 
	[ qi::_val = phx::bind(&Sketch::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5) ]
      
    ))
  );
}

}
}
