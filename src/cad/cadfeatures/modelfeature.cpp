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

#include "modelfeature.h"
#include "cadfeature.h"
#include "cadmodel.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{

defineType(ModelFeature);
addToFactoryTable(Feature, ModelFeature, NoParameters);

ModelFeature::ModelFeature(const NoParameters&): Compound()
{}


ModelFeature::ModelFeature(const std::string& modelname, const ModelVariableTable& vars)
: modelname_(modelname), vars_(vars)
{}

  
void ModelFeature::build()
{
  std::cout<<"loading model "<<modelname_<<std::endl;
  model_.reset(new Model(modelname_, vars_));
  model_->checkForBuildDuringAccess();
  
  BOOST_FOREACH(const Model::ComponentSet::value_type& c, model_->components())
  {
    std::cout<<"inserting component "<<c<<std::endl;
//     c.second->checkForBuildDuringAccess();
    components_[c]=model_->lookupModelstep(c);
  }
  BOOST_FOREACH(const Model::ModelstepTable::value_type& c, model_->modelsteps())
  {
    std::string name=c.first;
    // Compound::build copies the components. Here, the rest is copied.
    if (components_.find(name)==components_.end())
    {
      FeaturePtr p=c.second;
      
      copyDatums(*p, name);
      providedSubshapes_[name]=p;
    }
  }
  Compound::build();
}


void ModelFeature::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "loadmodel",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_identifier >> 
	*(',' >> (ruleset.r_identifier >> '=' >> (ruleset.r_scalarExpression|ruleset.r_vectorExpression) ) ) >> ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<ModelFeature>(qi::_1, qi::_2)) ]
      
    ))
  );
}

}
}