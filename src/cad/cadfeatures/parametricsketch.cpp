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

#include "parametricsketch.h"
#include "cadfeature.h"
#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"
#include "datum.h"
#include "base/boost_include.h"
#include "base/translations.h"

#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(ParametricSketch);
addToStaticFunctionTable(Feature, ParametricSketch, insertrule);
addToStaticFunctionTable(Feature, ParametricSketch, ruleDocumentation);



size_t ParametricSketch::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=DerivedFeature::calcHash();
  return h.getHash();
}



ParametricSketch::ParametricSketch(const ParametricSketch&o, TreeCloneMap& tcm)
    : DerivedFeature(o, tcm),
      scriptBegin_(o.scriptBegin_),
      scriptEnd_(o.scriptEnd_),
      hasScriptArgument_(o.hasScriptArgument_)
{}


ParametricSketch::ParametricSketch (
    std::shared_ptr<ConstrainedSketch> sketch,
    std::size_t scriptBegin,
    std::size_t scriptEnd,
    bool hasScriptArgument )
    : DerivedFeature ( sketch ),
      scriptBegin_(scriptBegin),
      scriptEnd_(scriptEnd),
      hasScriptArgument_(hasScriptArgument)
{
}




void ParametricSketch::build()
{
    setShape ( baseFeature()->shape() );
}




std::shared_ptr<const ConstrainedSketch> ParametricSketch::sketch() const
{
    return std::dynamic_pointer_cast<const ConstrainedSketch>(baseFeature());
}




std::pair<std::size_t,std::size_t> ParametricSketch::scriptRange() const
{
    return { scriptBegin_, scriptEnd_ };
}




bool ParametricSketch::hasScriptArgument() const
{
    return hasScriptArgument_;
}




void ParametricSketch::insertrule ( parser::ISCADParser& ruleset )
{
    typedef
        qi::rule<
            std::string::iterator,
            FeaturePtr(),
            parser::ISCADParser::skipper_type,
            qi::locals<
                std::shared_ptr<ConstrainedSketch>,
                std::shared_ptr<ConstrainedSketchGrammar>,
                std::size_t, std::size_t, bool >
            >
            ParametricSketchRule;

    auto *rule = new ParametricSketchRule(
        (
         '('

         > ruleset.r_datumExpression
             [ qi::_a = phx::bind(
                  &ConstrainedSketch::create<DatumPtr, const ConstrainedSketchParametersDelegate&>,
                  qi::_1, *noParametersDelegate),
               qi::_b = parser::make_shared_<ConstrainedSketchGrammar>()(
                  qi::_a, *noParametersDelegate,
                  phx::val(&ruleset)) ]

         > (
             (
               ',' > ruleset.current_pos.current_pos [ qi::_c = qi::_1 ]
               > qi::lazy(phx::bind(&ConstrainedSketchGrammar::r_sketch, qi::_b))
               > ruleset.current_pos.current_pos [ qi::_d = qi::_1, qi::_e = true ]
             )
             |
             ruleset.current_pos.current_pos [ qi::_c = qi::_1, qi::_d = qi::_1, qi::_e = false ]
           )

         > ')'
        )
        [ qi::_val = phx::bind(
            &ParametricSketch::create<std::shared_ptr<ConstrainedSketch>, std::size_t, std::size_t, bool>,
            qi::_a, qi::_c, qi::_d, qi::_e ) ]
    );
    ruleset.addAdditionalRule(rule);

    ruleset.modelstepFunctionRules.add
    (
        "ParametricSketch",
        std::make_shared<parser::ISCADParser::ModelstepRule>(*rule)
    );
}




FeatureCmdInfoList ParametricSketch::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "ParametricSketch",

            "( <datum:plane> [, <sketch script>] )",

            _("Creates a constrained sketch on the given plane. The sketch is defined by the sketch script."
              " The sketch script may be empty. It can be created and modified in the graphical sketch editor,"
              " which is accessible through the context menu of the script editor.")
        )
    };
}



}
}
