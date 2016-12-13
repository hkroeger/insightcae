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

#include "cadfeature.h"
#include "exploded.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "datum.h"

#include <limits.h>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
    
    
defineType(Exploded);
addToFactoryTable(Feature, Exploded);




Exploded::Exploded()
: Feature()
{}




Exploded::Exploded( DatumPtr axis, const ExplosionComponentList& m1)
: axis_(axis),
  components_(m1)
{}





Exploded::Exploded( DatumPtr axis, FeaturePtr assy)
: axis_(axis)
{
  for (int i=0; i<INT_MAX; i++)
  {
    std::string cname=str(format("component%d")%(i+1));
    try
    {
      components_.push_back(ExplosionComponent(assy->subshape(cname), ExplosionDirection_Axial, vec3const(0,0,0), scalarconst(1.)));
    }
    catch( ...)
    {
      break;
    }
  }
}





FeaturePtr Exploded::create( DatumPtr axis, const ExplosionComponentList& m1 )
{
    return FeaturePtr(new Exploded(axis, m1));
}


    
FeaturePtr Exploded::create_assy( DatumPtr axis, FeaturePtr assy )
{
    return FeaturePtr(new Exploded(axis, assy));
}
    
    
    
void Exploded::build()
{
    BRep_Builder bb;
    TopoDS_Compound result;
    bb.MakeCompound ( result );

    if (components_.size()>0)
    {
      if (!axis_->providesAxisReference())
	throw insight::Exception("Exploded: datum does not provide axis reference!");
      
      gp_Ax1 ax=axis_->axis();
      arma::mat p0=vec3(ax.Location()), dir=vec3(ax.Direction());
      
      typedef boost::tuple<ExplosionComponent,double,double,double> Loc;
      std::vector<Loc> axialOrdered;
      BOOST_FOREACH ( const ExplosionComponentList::value_type& c, components_ ) 
      {
	arma::mat cog=boost::fusion::at_c<0>(c)->modelCoG();
	arma::mat bb=boost::fusion::at_c<0>(c)->modelBndBox();
	double axloc = arma::dot( cog - p0, dir );
	double axmin = arma::dot( bb.col(0) - p0, dir );
	double axmax = arma::dot( bb.col(1) - p0, dir );
	axialOrdered.push_back(Loc(c, axloc, std::min(axmin,axmax), std::max(axmin,axmax) ));
      }
      
  //     std::sort
  //     (
  //       axialOrdered.begin(), axialOrdered.end(),
  //      
  //       [] (Loc const& a, Loc const& b) 
  //        { return boost::get<1>(a) < boost::get<1>(b); }
  //     );

      // first shape remains at its location, all further
      double lastmax=boost::fusion::at_c<3>(axialOrdered[0]);
      bb.Add ( result, *boost::fusion::at_c<0>(boost::fusion::at_c<0>(axialOrdered[0])) );

      for (size_t i=1; i<axialOrdered.size(); i++)
      {
	const Loc& c = axialOrdered[i];
	
	ExplosionComponent f = boost::fusion::at_c<0>(c);
// 	double axloc=boost::fusion::at_c<1>(c);
	double axmin=boost::fusion::at_c<2>(c);
	double axmax=boost::fusion::at_c<3>(c);
	double axextent=axmax-axmin;
	
// 	lastmax=std::max(axmin, lastmax);
	double fac=0.2*boost::fusion::at_c<3>(f)->value();
	double newmin = lastmax +fac*axextent;
	double curtrans = newmin - axmin;
	gp_Trsf tr;
	tr.SetTranslation(to_Vec( curtrans*dir ));
	bb.Add ( result, BRepBuilderAPI_Transform(*boost::fusion::at_c<0>(f), tr).Shape() );
	
	lastmax=axmax+curtrans;
      }
    }
    
    setShape ( result );
}




void Exploded::insertrule(parser::ISCADParser& ruleset) const
{
    ruleset.modelstepFunctionRules.add
    (
        "Exploded",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(
                      '(' 
                       > ( 
                        (
			 ( ( ruleset.r_solidmodel_expression 
                            > ( (qi::lit("axial")>qi::attr(ExplosionDirection_Axial)) 
			        | (qi::lit("radial")>qi::attr(ExplosionDirection_Radial)) 
				| qi::attr(ExplosionDirection_Axial) )
                            > (ruleset.r_vectorExpression|qi::attr(vec3const(0,0,0))) 
                            > (ruleset.r_scalarExpression|qi::attr(scalarconst(1.))) 
			   ) % ',' )
			 > ',' > ruleset.r_datumExpression > ')' )
                      [ qi::_val = phx::bind(&Exploded::create, qi::_2, qi::_1) ]
                        |
                        
                        (qi::lit("assembly") > ruleset.r_solidmodel_expression > ',' > ruleset.r_datumExpression > ')' )
                      [ qi::_val = phx::bind(&Exploded::create_assy, qi::_2, qi::_1) ]
                     ) 
                ))
    );
}




FeatureCmdInfoList Exploded::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Exploded",
         
            "( <datum:refaxis>, <feature:c0> [, ..., <feature:cn> ] )",
         
            "Creates an exploded state from the supplied list of features. The components are translated along the direction of refaxis."
        )
    );
}





}
}
