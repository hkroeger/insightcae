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

#include "bar.h"
#include "quad.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

  
defineType(Bar);
addToFactoryTable(SolidModel, Bar, NoParameters);

void Bar::create
(
  const arma::mat& start, 
  const arma::mat& end, 
  const SolidModel& xsec, 
  const arma::mat& vert, 
  double ext0, double ext1, 
  double miterangle0_vert, double miterangle1_vert, 
  double miterangle0_hor, double miterangle1_hor
)
{
  
  ParameterListHash h;
  h+=start;
  h+=end;
  h+=xsec;
  h+=vert;
  h+=ext0;
  h+=ext1;
  h+=miterangle0_vert;
  h+=miterangle1_vert;
  h+=miterangle0_hor;
  h+=miterangle1_hor;
  
  if (!cache.contains(h))
  {

    arma::mat p0=start;
    arma::mat p1=end;
    
    if (norm(vert,2)<1e-10)
      throw insight::Exception("Bar: length of vertical direction is zero!");
    arma::mat v=vert/norm(vert,2);
    
    if (!xsec.isSingleFace() || xsec.isSingleWire() || xsec.isSingleEdge())
      throw insight::Exception("xsec feature has to provide a face or wire!");
    
    arma::mat baraxis=p1-p0;
    double lba=norm(baraxis,2);
    if (lba<1e-10)
      throw insight::Exception("Bar: invalid definition of bar end points!");
    baraxis/=lba;
    
    p0 += -baraxis*ext0;
    p1 +=  baraxis*ext1;
    double L=norm(p1-p0, 2);
    
    refpoints_["start"]=start;
    refpoints_["end"]=end;
    refpoints_["p0"]=p0;
    refpoints_["p1"]=p1;
    
    refvalues_["L"]=L;

    TopoDS_Wire spine=BRepBuilderAPI_MakeWire
    (
      BRepBuilderAPI_MakeEdge
      (
	GC_MakeSegment(to_Pnt(p0), to_Pnt(p1))
      )
    );
  //   TopoDS_Vertex pfirst, plast;
  //   TopExp::Vertices( spine, pfirst, plast );
    
      
    arma::mat ex=-arma::cross(baraxis, vert);
    
    double lex=norm(ex, 2);
    if (lex<1e-10)
      throw insight::Exception("Bar: invalid definition of vertical direction!");
    ex/=lex;
    
    arma::mat ey=arma::cross(baraxis, ex);
    
    gp_Trsf tr;
    tr.SetTransformation
    (
      // from
      gp_Ax3
      (
	gp_Pnt(0,0,0),
	gp_Dir(0,0,1),
	gp_Dir(1,0,0)
      ),
      //to
      gp_Ax3
      (
	to_Pnt(p0),
	to_Vec(baraxis),
	to_Vec(ex)
      )
    );
    TopoDS_Shape xsecs=BRepBuilderAPI_Transform(static_cast<TopoDS_Shape>(xsec), tr.Inverted()).Shape();

  //   BRepOffsetAPI_MakePipeShell p(spinew);
  //   Handle_Law_Constant law(new Law_Constant());
  //   law->Set(1.0, -1e10, 1e10);
  //   p.SetLaw(static_cast<TopoDS_Shape>(xsec), law, pfirst);
  //   p.SetMode(true);
  //   p.MakeSolid();
    
    BRepOffsetAPI_MakePipe p(spine, xsecs);
    
    p.Build();
    TopoDS_Shape result=p.Shape();
    
    // cut away at end 0
    if ( (fabs(miterangle0_vert)>1e-10) || (fabs(miterangle0_hor)>1e-10) )
    {
      arma::mat cex=rotMatrix(miterangle0_vert, ey)*ex;
      arma::mat cey=rotMatrix(miterangle0_hor, ex)*ey;
      Quad q(start-0.5*L*(cex+cey), L*cex, L*cey);
      TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(-L*baraxis) );
      result=BRepAlgoAPI_Cut(result, airspace);
    }
    
    // cut away at end 1
    if ( (fabs(miterangle1_vert)>1e-10) || (fabs(miterangle1_hor)>1e-10) )
    {
      arma::mat cex=rotMatrix(miterangle1_vert, ey)*ex;
      arma::mat cey=rotMatrix(miterangle1_hor, ex)*ey;
      Quad q(end-0.5*L*(cex+cey), L*cex, L*cey);
      TopoDS_Shape airspace=BRepPrimAPI_MakePrism(TopoDS::Face(q), to_Vec(L*baraxis) );
      result=BRepAlgoAPI_Cut(result, airspace);
    }
    
    setShape(result);
    
    write(cache.markAsUsed(h));
  }
  else
  {
    read(cache.markAsUsed(h));
  }
}


Bar::Bar(const NoParameters& nop): SolidModel(nop)
{}


Bar::Bar
(
  const arma::mat& start, const arma::mat& end, 
  const SolidModel& xsec, const arma::mat& vert, 
  double ext0, double ext1,
  double miterangle0_vert, double miterangle1_vert,
  double miterangle0_hor, double miterangle1_hor
)
{
  create(start, end, xsec, vert, ext0, ext1, miterangle0_vert, miterangle1_vert, miterangle0_hor, miterangle1_hor);
}

Bar::Bar
(
  const arma::mat& start, const arma::mat& end, 
  const SolidModel& xsec, const arma::mat& vert, 
  const boost::fusion::vector3<double,double,double>& ext_miterv_miterh0, 
  const boost::fusion::vector3<double,double,double>& ext_miterv_miterh1
)
: SolidModel(xsec)
{
  create
  (
    start, end, 
    xsec, vert, 
    boost::fusion::at_c<0>(ext_miterv_miterh0), boost::fusion::at_c<0>(ext_miterv_miterh1),
    boost::fusion::at_c<1>(ext_miterv_miterh0), boost::fusion::at_c<1>(ext_miterv_miterh1),
    boost::fusion::at_c<2>(ext_miterv_miterh0), boost::fusion::at_c<2>(ext_miterv_miterh1)
  );
}


void Bar::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Bar",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
	>> ruleset.r_vectorExpression // 1
	  >> qi::hold[ (  (( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(0.0)) 
	  >> ((  qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0))  
 	  >> ((  qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0))  ) ]
	  >> ',' 
	>> ruleset.r_vectorExpression // 3
	  >> qi::hold[ (  (( qi::lit("ext") >> ruleset.r_scalarExpression ) | qi::attr(0.0))  
	  >> ((  qi::lit("vmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0))  
 	  >> ((  qi::lit("hmiter") >> ruleset.r_scalarExpression ) | qi::attr(0.0))  ) ]
	  >> ',' 
	>> ruleset.r_solidmodel_expression >> ',' // 5
	>> ruleset.r_vectorExpression >> // 6
      ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Bar>
	(
	  qi::_1, qi::_3, 
	  *qi::_5, qi::_6,
	  qi::_2, qi::_4
	)) ]
      
    ))
  );
}

}
}
