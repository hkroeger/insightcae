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

#include <chrono>

using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


namespace insight {
namespace cad {


defineType(Sketch);
addToFactoryTable(Feature, Sketch);

size_t Sketch::calcHash() const
{
  ParameterListHash p;
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
  return p.getHash();
}


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
{}





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
    insight::CurrentExceptionContext ex("building sketch "+featureSymbolName());
    ExecTimer t("Sketch::build() ["+featureSymbolName()+"]");
    
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
#warning behaviour in freecad has changed
            //layername="0";

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
            std::cout<<"CMD=\""<<cmd<<"\""<<std::endl;
            if ( ::system( cmd.c_str() ) || !boost::filesystem::exists(filename) )
            {
                throw insight::Exception("Conversion of pSketch file "+infilename.string()+" into DXF "+filename.string()+" failed!");
            }
        }

//        TopoDS_Wire w = DXFReader(filename, layername).Wire(tol_);
//        providedSubshapes_["OuterWire"]=FeaturePtr(new Feature(w));
        auto ws = DXFReader(filename, ".*").Wires(tol_);
        if (ws->Size()==1)
        {
            providedSubshapes_["OuterWire"]=FeaturePtr(new Feature(ws->Value(1)));
        }
        else
        {
            for (int i=0; i<ws->Size(); ++i)
            {
                providedSubshapes_[
                        str(format("OuterWire%d")%(i+1))]=
                        FeaturePtr(new Feature(ws->Value(i+1)));
            }
        }

        gp_Trsf tr;
        gp_Ax3 ax=*pl_;
        tr.SetTransformation(ax);

        BRep_Builder bb;
        TopoDS_Compound result;
        bb.MakeCompound ( result );
        for (auto w=ws->begin(); w!=ws->end(); ++w)
        {
            BRepBuilderAPI_Transform btr(*w, tr.Inverted(), true);

            if (w->Closed())
                bb.Add ( result, BRepBuilderAPI_MakeFace(
                             gp_Pln(ax),
                             TopoDS::Wire(btr.Shape())).Shape());
            else
                bb.Add ( result, TopoDS::Wire(btr.Shape()));
        }

        setShape(result);

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<Sketch>(hash()));
    }
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










//SketchPoint::SketchPoint(DatumPtr planei, const arma::mat &p3)
//    : plane_(planei),
//      x_(0), y_(0)
//{
//    auto plane=plane_->plane();
//    gp_Trsf pl;
//    pl.SetTransformation(plane); // from global to plane
//    auto pp = toVec<gp_Pnt>(p3).Transformed(pl);
//    insight::assertion(
//                fabs(pp.Z())<SMALL,
//                "point (%g, %g, %g) not in plane with p=(%g, %g, %g) and n=(%g, %g, %g)!\n"
//                "(local coordinates = (%g, %g, %g))",
//                p3(0), p3(1), p3(2),
//                plane.Location().X(), plane.Location().Y(), plane.Location().Z(),
//                plane.Direction().X(), plane.Direction().Y(), plane.Direction().Z(),
//                pp.X(), pp.Y(), pp.Z() );
//    x_=pp.X();
//    y_=pp.Y();
//}

SketchPoint::SketchPoint(DatumPtr plane, double x, double y)
    : plane_(plane),
      x_(x), y_(y)
{}

void SketchPoint::setCoords2D(double x, double y)
{
    x_=x;
    y_=y;
}

arma::mat SketchPoint::coords2D() const
{
    return vec2(x_, y_);
}

arma::mat SketchPoint::value() const
{
    auto pl=plane_->plane();
    return vec3(
        pl.Location()
            .Translated(pl.XDirection().XYZ()*x_)
            .Translated(pl.YDirection().XYZ()*y_)
                );
}

int SketchPoint::nDoF() const
{
    return 2;
}


double SketchPoint::getDoFValue(unsigned int iDoF) const
{
    insight::assertion(
                iDoF<nDoF(),
                "invalid DoF index: %d", iDoF );
    switch (iDoF)
    {
        case 0: return x_;
        case 1: return y_;
    }
    return NAN;
}


void SketchPoint::setDoFValue(unsigned int iDoF, double value)
{
    insight::assertion(
                iDoF<nDoF(),
                "invalid DoF index: %d", iDoF );
    switch (iDoF)
    {
        case 0: x_=value;
        case 1: y_=value;
        }
}

void SketchPoint::scaleSketch(double scaleFactor)
{
        x_*=scaleFactor;
        y_*=scaleFactor;
}







defineType(ConstrainedSketch);
//addToFactoryTable(Feature, ConstrainedSketch);


size_t ConstrainedSketch::calcHash() const
{
  ParameterListHash p;
  p+=*pl_;
  for (const auto& se: geometry_)
      p+=se->hash();
  return p.getHash();
}



ConstrainedSketch::ConstrainedSketch()
: Feature(),
  solverTolerance_(1e-6)
{}



ConstrainedSketch::ConstrainedSketch
(
  DatumPtr pl
)
: pl_(pl),
  solverTolerance_(1e-6)
{}




FeaturePtr ConstrainedSketch::create( DatumPtr pl )
{
    return FeaturePtr(new ConstrainedSketch(pl));
}


const DatumPtr& ConstrainedSketch::plane() const
{
    return pl_;
}


std::set<std::shared_ptr<ConstrainedSketchEntity> >&
ConstrainedSketch::geometry()
{
    return geometry_;
}

void ConstrainedSketch::operator=(const ConstrainedSketch &o)
{
    Feature::operator=(o);
    pl_=o.pl_;
    geometry_=o.geometry_;
}

double ConstrainedSketch::solverTolerance() const
{
    return solverTolerance_;
}

void ConstrainedSketch::setSolverTolerance(double tol)
{
    solverTolerance_=tol;
}

void ConstrainedSketch::resolveConstraints()
{
    std::vector<std::pair<insight::cad::ConstrainedSketchEntity*, int> > dofs;
    for (auto& e: geometry())
    {
        for (int i=0; i<e->nDoF(); ++i)
        {
            dofs.push_back({e.get(), i});
        }
    }

    arma::mat x0=arma::zeros(dofs.size());
    for (int i=0; i<dofs.size(); ++i)
    {
        x0(i)=dofs[i].first->getDoFValue(dofs[i].second);
    }
    std::cout<<"x0="<<x0<<std::endl;

    auto setX = [&](const arma::mat& x)
    {
        for (int i=0; i<dofs.size(); ++i)
        {
            dofs[i].first->setDoFValue(dofs[i].second, x(i));
        }
        for (auto& se: geometry())
        {
            if ( auto e = std::dynamic_pointer_cast<ASTBase>(se) )
            {
                e->invalidate();
            }
        }
    };

    arma::mat xsol = nonlinearMinimizeND(
                [&](const arma::mat& x) -> double
                {
                    setX(x);
                    double Q=0;
                    for (auto& e: geometry())
                    {
                        for (int i=0;i<e->nConstraints();++i)
                        {
                            Q+=pow(e->getConstraintError(i), 2);
                        }
                    }
                    std::cout<<"Q="<<Q<<std::endl;
                    return Q;
                },
                x0, solverTolerance_
    );

    setX(xsol);
}




void ConstrainedSketch::build()
{
    ExecTimer t("ConstrainedSketch::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        if (!pl_->providesPlanarReference())
            throw insight::Exception("Sketch: Planar reference required!");

        BRep_Builder bb;
        TopoDS_Compound result;
        bb.MakeCompound ( result );

        for ( auto& sg: geometry_ )
        {
            if ( auto f = std::dynamic_pointer_cast<Feature>(sg) )
            {
                bb.Add ( result, f->shape() );
            }
        }

        setShape(result);

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<ConstrainedSketch>(hash()));
    }
}










}
}
