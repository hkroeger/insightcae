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

#include "wire.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

    
    
    
defineType(Wire);
addToFactoryTable(Feature, Wire);


size_t Wire::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  if (const auto* e1=boost::get<FeatureSetPtr>(&edges_))
  {
      h+=*(*e1);
  }
  else if (const auto* e2=boost::get<const std::vector<FeaturePtr> >(&edges_))
  {
      for (const auto& e: *e2)
        h+=*e;
  }
  return h.getHash();
}



Wire::Wire(): Feature()
{}




Wire::Wire(FeatureSetPtr edges)
: edges_(edges)
{}


Wire::Wire(const std::vector<FeaturePtr>& edges)
    : edges_(edges)
{
}


FeaturePtr Wire::create(FeatureSetPtr edges)
{
  return FeaturePtr(new Wire(edges));
}

FeaturePtr Wire::create_feats(const std::vector<FeaturePtr>& edges)
{
  return FeaturePtr(new Wire(edges));
}




void Wire::build()
{
  TopTools_ListOfShape ee;

  if (const auto* e1=boost::get<FeatureSetPtr>(&edges_))
  {
      for ( const FeatureID& fi: (*e1)->data() )
      {
        ee.Append ( (*e1)->model()->edge ( fi ) );
      }
  }
  else if (const auto* e2=boost::get<const std::vector<FeaturePtr> >(&edges_))
  {
      for ( const auto& fp: (*e2))
      {
        auto ae=fp->allEdges();
        for (const auto& ei: ae->data())
        {
            ee.Append ( fp->edge ( ei ) );
        }
      }
  }

    BRepBuilderAPI_MakeWire wb;
    wb.Add ( ee );
    
    ShapeFix_Wire wf;
    wf.Load(wb.Wire());
    wf.Perform();
    
    setShape ( wf.Wire() );
}




void Wire::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Wire",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    '(' > (
        ( ruleset.r_edgeFeaturesExpression )
          [ qi::_val = phx::bind(&Wire::create, qi::_1) ]
        |
        ( ruleset.r_modelstep % ',' )
            [ qi::_val = phx::bind(&Wire::create_feats, qi::_1) ]
     ) > ')'
    ))
  );
}




FeatureCmdInfoList Wire::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Wire",
            "( <edgeSelection> )",
            "Creates a wire from a number of edges."
        )
    );
}



bool Wire::isSingleClosedWire() const
{
  return TopoDS::Wire(shape()).Closed();
}




bool Wire::isSingleOpenWire() const
{
  return !isSingleClosedWire();
}




}
}
