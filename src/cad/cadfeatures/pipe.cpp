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

#include "pipe.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

  
defineType(Pipe);
addToFactoryTable(Feature, Pipe, NoParameters);

Pipe::Pipe(const NoParameters& nop): Feature(nop)
{}


Pipe::Pipe(FeaturePtr spine, FeaturePtr xsec, bool orient)
: spine_(spine), xsec_(xsec), orient_(orient)
{}

void Pipe::build()
{
  if (!spine_->isSingleWire())
    throw insight::Exception("spine feature has to provide a singly connected wire!");  // not working for wires created from feature edge selection
  
  if (!xsec_->isSingleFace() || xsec_->isSingleWire() || xsec_->isSingleEdge())
    throw insight::Exception("xsec feature has to provide a face or wire!");

  TopoDS_Wire spinew=spine_->asSingleWire();

//   TopoDS_Vertex pfirst, plast;
//   TopExp::Vertices( spinew, pfirst, plast );
  
  BRepAdaptor_CompCurve w(spinew);
  double p0=w.FirstParameter();
  double p1=w.LastParameter();
  
  
  TopoDS_Shape xsec=xsec_->shape();

  gp_Trsf tr;
//   tr.SetTranslation(gp_Vec(BRep_Tool::Pnt(pfirst).XYZ()));
  if (!orient_)
    tr.SetTranslation(w.Value(p0).XYZ());
  else
  {
    gp_Pnt v0;
    gp_Vec vp0;
    w.D1(p0, v0, vp0);
    vp0.Normalize();
    std::cout<<v0.X()<<" "<<v0.Y()<<" "<<v0.Z()<<endl;
    std::cout<<vp0.X()<<" "<<vp0.Y()<<" "<<vp0.Z()<<endl;
    gp_Trsf tr1;
    tr1.SetTransformation
    (
      gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(1,0,0)),
      gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(vp0))
    );
    xsec=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr1).Shape();
// 
     tr.SetTranslationPart(v0.XYZ());
//     tr.SetAxis(gp_Ax1(v0, gp_Dir(vp0)));
  }
  TopoDS_Shape xsecs=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr).Shape();
  
//   BRepOffsetAPI_MakePipeShell p(spinew);
//   Handle_Law_Constant law(new Law_Constant());
//   law->Set(1.0, -1e10, 1e10);
//   p.SetLaw(static_cast<TopoDS_Shape>(xsec), law, pfirst);
//   p.SetMode(true);
//   p.MakeSolid();
  
  BRepOffsetAPI_MakePipe p(spinew, xsecs);
  
  p.Build();
  setShape(p.Shape());
}

void Pipe::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Pipe",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' 
          >> ruleset.r_solidmodel_expression 
          >> ( ( ',' >> qi::lit("orient") >> qi::attr(true) ) | qi::attr(false) ) >> ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Pipe>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
