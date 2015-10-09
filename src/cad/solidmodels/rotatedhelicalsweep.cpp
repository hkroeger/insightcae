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

#include "rotatedhelicalsweep.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {



defineType(RotatedHelicalSweep);
addToFactoryTable(SolidModel, RotatedHelicalSweep, NoParameters);

RotatedHelicalSweep::RotatedHelicalSweep(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape makeRotatedHelicalSweep(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double P, double revoffset)
{
//   BRep_Builder bb;
//   TopoDS_Compound result;
//   bb.MakeCompound(result);

  BRepOffsetAPI_ThruSections sb(true);
  
  TopoDS_Wire ow=BRepTools::OuterWire(TopoDS::Face(sk));
  
  double dz=arma::norm(axis, 2);
  arma::mat ez=axis/norm(axis, 2);
  double phi=0.0, dphi=2.*dz*M_PI/P;
  int nstep=std::max( 2, int(ceil(dphi/(M_PI/64.))) );
  double phi_step=dphi/double(nstep);
  
#define TRSF(shape, deltaz, oshape) \
  {\
    gp_Trsf t1, t2;\
    t1.SetTranslation( to_Vec(ez).Scaled(deltaz) );\
    t2.SetRotation( gp_Ax1( to_Pnt(p0), to_Vec(ez) ), 2.*deltaz*M_PI/P );\
    oshape = TopoDS::Wire(BRepBuilderAPI_Transform\
      (\
	BRepBuilderAPI_Transform\
	(\
	  shape, \
	  t1\
	).Shape(),\
        t2\
      ).Shape());\
  }
  
  TopoDS_Wire firstsec;
  TRSF(ow, -revoffset, firstsec);
  
  for (int i=0; i<nstep+1; i++)
  {
    double z=phi*P/(2.*M_PI);
    TopoDS_Wire cursec;
    TRSF(firstsec, z, cursec);
//     bb.Add(result, cursec);
    sb.AddWire(cursec);
    
    phi+=phi_step;
  }
  
//   return result;
  return sb.Shape();
}


RotatedHelicalSweep::RotatedHelicalSweep(const SolidModel& sk, const arma::mat& p0, const arma::mat& axis, double P, double revoffset)
: SolidModel(makeRotatedHelicalSweep(sk, p0, axis, P, revoffset))
{
}

void RotatedHelicalSweep::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "RotatedHelicalSweep",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
	    > ruleset.r_solidmodel_expression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_vectorExpression > ',' 
	    > ruleset.r_scalarExpression > 
	    ((  ',' > ruleset.r_scalarExpression ) | qi::attr(0.0)) > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<RotatedHelicalSweep>(*qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}


}
}