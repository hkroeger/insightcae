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
addToFactoryTable(Feature, RotatedHelicalSweep);

RotatedHelicalSweep::RotatedHelicalSweep(): Feature()
{}


TopoDS_Shape makeRotatedHelicalSweep(const Feature& sk, const arma::mat& p0, const arma::mat& axis, double P, double revoffset)
{
//   BRep_Builder bb;
//   TopoDS_Compound result;
//   bb.MakeCompound(result);

  BRepOffsetAPI_ThruSections sb(true);
  
  TopoDS_Wire ow;
  
  if (sk.isSingleFace())
  {
      ow=BRepTools::OuterWire(TopoDS::Face(sk));
  } else if (sk.isSingleWire())
  {
      ow=sk.asSingleWire();
  } else
  {
      throw insight::Exception("Incompatible input shape for RotatedHelicalSweep!");
  }
  
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


RotatedHelicalSweep::RotatedHelicalSweep(FeaturePtr sk, VectorPtr p0, VectorPtr axis, ScalarPtr P, ScalarPtr revoffset)
: sk_(sk), p0_(p0), axis_(axis), P_(P), revoffset_(revoffset)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*sk_;
  h+=p0_->value();
  h+=axis_->value();
  h+=P_->value();
  h+=revoffset_->value();
}

void RotatedHelicalSweep::build()
{
  setShape(makeRotatedHelicalSweep(*sk_, p0_->value(), axis_->value(), P_->value(), revoffset_->value()));
}

void RotatedHelicalSweep::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "RotatedHelicalSweep",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
	    >> ruleset.r_solidmodel_expression >> ',' 
	    >> ruleset.r_vectorExpression >> ',' 
	    >> ruleset.r_vectorExpression >> ',' 
	    >> ruleset.r_scalarExpression >> 
	    ((  ',' >> ruleset.r_scalarExpression ) | qi::attr(scalarconst(0.0))) >> ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<RotatedHelicalSweep>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}


}
}
