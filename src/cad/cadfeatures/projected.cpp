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

#include "projected.h"
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


    
    
defineType(Projected);
//addToFactoryTable(Feature, Projected);
addToStaticFunctionTable(Feature, Projected, insertrule);
addToStaticFunctionTable(Feature, Projected, ruleDocumentation);

size_t Projected::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*source_;
  h+=*target_;
  h+=dir_->value();
  return h.getHash();
}






Projected::Projected(FeaturePtr source, FeaturePtr target, VectorPtr dir)
: source_(source), target_(target), dir_(dir)
{}







void Projected::build()
{
    TopoDS_Shape ow;
    if (TopExp_Explorer(source_->shape(), TopAbs_FACE).More())
    {
        // face
        ow=BRepTools::OuterWire(TopoDS::Face(source_->shape()));
    }
    else
    {
        // face
        ow=source_->shape();
    }

    BRepProj_Projection proj(ow, target_->shape(), to_Vec(dir_->value()));

    TopTools_ListOfShape ee;
    TopoDS_Shape projs = proj.Shape();
    for (TopExp_Explorer ex(projs, TopAbs_EDGE); ex.More(); ex.Next())
    {
        ee.Append ( TopoDS::Edge(ex.Current()) );
    }
    BRepBuilderAPI_MakeWire wb;
    wb.Add ( ee );

    ShapeFix_Wire wf;
    wf.Load(wb.Wire());
    wf.Perform();
    
    setShape(/*proj.Shape()*/wf.Wire());
}




void Projected::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "Projected",	
    std::make_shared<parser::ISCADParser::ModelstepRule>(

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
      [ qi::_val = phx::bind(
                         &Projected::create<FeaturePtr, FeaturePtr, VectorPtr>,
                         qi::_1, qi::_2, qi::_3) ]
      
    )
  );
}




FeatureCmdInfoList Projected::ruleDocumentation()
{
  return {
        FeatureCmdInfo
        (
            "Projected",
            "( <feature:base>, <feature:target>, <vector:dir> )",
          _("Projects the feature base onto the feature target. The direction has to be prescribed by vector dir.")
        )
    };
}


bool Projected::isSingleClosedWire() const
{
  return TopoDS::Wire(shape()).Closed();
}




bool Projected::isSingleOpenWire() const
{
  return !isSingleClosedWire();
}

}
}
