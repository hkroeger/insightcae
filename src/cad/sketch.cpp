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
