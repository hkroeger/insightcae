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

#include "mirror.h"
#include "datum.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/translations.h"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    

defineType(Mirror);
//addToFactoryTable(Feature, Mirror);
addToStaticFunctionTable(Feature, Mirror, insertrule);
addToStaticFunctionTable(Feature, Mirror, ruleDocumentation);



size_t Mirror::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*m1_;
  if (pl_)
    {
      h+=*pl_;
    }
  else
    {
      h+=int(s_);
    }
  return h.getHash();
}






Mirror::Mirror(FeaturePtr m1, DatumPtr pl)
: DerivedFeature(m1), m1_(m1), pl_(pl)
{}




Mirror::Mirror(FeaturePtr m1, Mirror::Shortcut s)
: DerivedFeature(m1), m1_(m1), s_(s)
{}






void Mirror::build()
{

    if ( pl_ ) {
        if ( !pl_->providesPlanarReference() ) {
          throw insight::Exception ( _("Mirror: planar reference required!") );
        }

        tr_.SetMirror ( static_cast<gp_Ax3> ( *pl_ ).Ax2() );
    } else if ( s_==FlipY ) {
        tr_.SetMirror ( gp_Ax2 ( gp_Pnt ( 0,0,0 ), gp_Dir ( 0,1,0 ) ) );
    } else if ( s_==FlipX ) {
        tr_.SetMirror ( gp_Ax2 ( gp_Pnt ( 0,0,0 ), gp_Dir ( 1,0,0 ) ) );
    } else if ( s_==FlipXY ) {
        tr_.SetMirror ( gp_Ax2 ( gp_Pnt ( 0,0,0 ), gp_Dir ( 1,1,0 ) ) );
    }

    TopoDS_Shape ms = BRepBuilderAPI_Transform ( m1_->shape(), tr_ ).Shape();
    ShapeFix_Shape fix;
    fix.Init( ms );
    fix.Perform();
    setShape ( fix.Shape() );
    copyDatumsTransformed ( *m1_, tr_ );
}




void Mirror::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "Mirror",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_datumExpression >> ')' )
       [ qi::_val = phx::bind(&Mirror::create<FeaturePtr, DatumPtr>, qi::_1, qi::_2) ]
      
    ))
  );
  ruleset.modelstepFunctionRules.add
  (
    "FlipY",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ')' )
      [ qi::_val = phx::bind(&Mirror::create<FeaturePtr, Mirror::Shortcut>, qi::_1, FlipY) ]
      
    ))
  );
  ruleset.modelstepFunctionRules.add
  (
    "FlipX",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ')' ) 
      [ qi::_val = phx::bind(&Mirror::create<FeaturePtr, Mirror::Shortcut>, qi::_1, FlipX) ]
      
    ))
  );
  ruleset.modelstepFunctionRules.add
  (
    "FlipXY",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ')' ) 
      [ qi::_val = phx::bind(&Mirror::create<FeaturePtr, Mirror::Shortcut>, qi::_1, FlipXY) ]
      
    ))
  );
}




FeatureCmdInfoList Mirror::ruleDocumentation()
{
  return {
        FeatureCmdInfo
        (
            "Mirror",
            "( <feature:base>, <datum:plane> )",
          _("Mirrors the base feature base over the given datum plane.")
        ),
        FeatureCmdInfo
        (
            "FlipX",
            "( <feature> )",
          _("Mirrors the base feature over the YZ plane.")
        ),
        FeatureCmdInfo
        (
            "FlipY",
            "( <feature> )",
          _("Mirrors the base feature over the XZ plane.")
        ),
        FeatureCmdInfo
        (
            "FlipXY",
            "( <feature> )",
          _("Mirrors the base feature over the diagonal plane with n=[1,1,0].")
        )
    };
}



gp_Trsf Mirror::transformation() const
{
  checkForBuildDuringAccess();
  return tr_;
}





}
}
