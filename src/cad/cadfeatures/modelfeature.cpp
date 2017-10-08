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
#include "datum.h"
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
addToFactoryTable(Feature, ModelFeature);




void ModelFeature::copyModelDatums()
{
  auto scalars=model_->scalars();
  BOOST_FOREACH(decltype(scalars)::value_type const& v, scalars)
  {
    if (refvalues_.find(v.first)!=refvalues_.end())
      throw insight::Exception("datum value "+v.first+" already present!");
    refvalues_[v.first]=v.second->value();
  }
  auto vectors=model_->vectors();
  BOOST_FOREACH(decltype(vectors)::value_type const& p, vectors)
  {
    if (refpoints_.find(p.first)!=refpoints_.end())
      throw insight::Exception("datum point "+p.first+" already present!");
    refpoints_[p.first]=p.second->value();
  }

  auto datums=model_->datums();
  BOOST_FOREACH(decltype(datums)::value_type const& d, datums)
  {
    if (providedDatums_.find(d.first)!=providedDatums_.end())
      throw insight::Exception("datum "+d.first+" already present!");

    providedDatums_[d.first]=d.second;
  }
}




ModelFeature::ModelFeature(): Compound()
{}




ModelFeature::ModelFeature(const std::string& modelname, const ModelVariableTable& vars)
: modelinput_(modelname), vars_(vars)
{
  // build the parameter hash
  ParameterListHash p(this);
  p+=this->type();
  
  std::string fname=modelname+".iscad";
  try 
  {
    // try to incorporate file time stamp etc
    p+=sharedModelFilePath(fname);
  }
  catch (...)
  {
    // if file is non-existing, use filename only
    p+=fname;
  }

  for (ModelVariableTable::const_iterator it=vars.begin(); it!=vars.end(); it++)
  {
    p+=boost::fusion::at_c<0>(*it);
    auto v=boost::fusion::at_c<1>(*it);
    if (FeaturePtr* fp = boost::get<FeaturePtr>(&v))
    {
        p+=(*fp);
    }
    else if (DatumPtr* dp = boost::get<DatumPtr>(&v))
    {
        p+=(*dp);
    }
    else if (VectorPtr* vp = boost::get<VectorPtr>(&v))
    {
        p+=(*vp)->value();
    }
    else if (ScalarPtr* sp = boost::get<ScalarPtr>(&v))
    {
        p+=(*sp)->value();
    }
    
  }
}




ModelFeature::ModelFeature(const boost::filesystem::path& modelfile, const ModelVariableTable& vars)
: modelinput_(modelfile), vars_(vars)
{
  // build the parameter hash
  ParameterListHash p(this);
  p+=this->type();
  p+=modelfile;

  for (ModelVariableTable::const_iterator it=vars.begin(); it!=vars.end(); it++)
  {
    p+=boost::fusion::at_c<0>(*it);
    auto v=boost::fusion::at_c<1>(*it);
    if (FeaturePtr* fp = boost::get<FeaturePtr>(&v))
    {
        p+=(*fp);
    }
    else if (DatumPtr* dp = boost::get<DatumPtr>(&v))
    {
        p+=(*dp);
    }
    else if (VectorPtr* vp = boost::get<VectorPtr>(&v))
    {
        p+=(*vp)->value();
    }
    else if (ScalarPtr* sp = boost::get<ScalarPtr>(&v))
    {
        p+=(*sp)->value();
    }
  }
}




ModelFeature::ModelFeature(ModelPtr model)
: model_(model)
{
}



FeaturePtr ModelFeature::create(const std::string& modelname, const ModelVariableTable& vars)
{
    return FeaturePtr
           (
               new ModelFeature
               (
                   modelname, vars
               )
           );
}


FeaturePtr ModelFeature::create_file(const boost::filesystem::path& modelfile, const ModelVariableTable& vars)
{
    return FeaturePtr
           (
               new ModelFeature
               (
                   modelfile, vars
               )
           );
}


FeaturePtr ModelFeature::create_model(ModelPtr model)
{
  return FeaturePtr(new ModelFeature(model));
}


void ModelFeature::build()
{
    if (!cache.contains(hash()))
    {
	if (!model_)
	{
	  if (boost::filesystem::path* fp = boost::get<boost::filesystem::path>(&modelinput_))
	  {
	    model_.reset(new Model(*fp, vars_));
	  } else if (std::string* mn = boost::get<std::string>(&modelinput_))
	  {
	    model_.reset(new Model(*mn, vars_));
	  } else
	  {
	    throw insight::Exception("ModelFeature: Model input unspecified!");
	  }
	}
        model_->checkForBuildDuringAccess();

        BOOST_FOREACH(const Model::ComponentSet::value_type& c, model_->components())
        {
            components_[c]=model_->lookupModelstep(c);
        }

        auto modelsteps=model_->modelsteps();
        BOOST_FOREACH(decltype(modelsteps)::value_type const& c, modelsteps)
        {
            std::string name=c.first;
            // Compound::build copies the components. Here, the rest is copied.
            if (components_.find(name)==components_.end())
            {
                FeaturePtr p=c.second;

//                 copyDatums(*p, name+"_");
                providedSubshapes_[name]=p;
            }
        }

        copyModelDatums();

        Compound::build();

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<ModelFeature>(hash()));
    }
}


std::string ModelFeature::modelname() const
{
    if (boost::filesystem::path* fp = boost::get<boost::filesystem::path>(&modelinput_))
    {
      std::string fname = boost::filesystem::basename(*fp);
      boost::erase_last(fname, ".iscad");
      return fname;
    } 
    else if (std::string* mn = boost::get<std::string>(&modelinput_))
    {
      return *mn;
    } 
    else
    {
      throw insight::Exception("ModelFeature: Model input unspecified!");
      return std::string();
    }
}


boost::filesystem::path ModelFeature::modelfile() const
{
    if (boost::filesystem::path* fp = boost::get<boost::filesystem::path>(&modelinput_))
    {
      return boost::filesystem::absolute(*fp);
    } 
    else if (std::string* mn = boost::get<std::string>(&modelinput_))
    {
      return boost::filesystem::absolute(sharedModelFilePath(*mn+".iscad"));
    } 
    else
    {
      throw insight::Exception("ModelFeature: Model input unspecified!");
      return boost::filesystem::path();
    }
}


void ModelFeature::executeEditor()
{
//   std::string name=modelname_+".iscad";
  boost::filesystem::path fp = modelfile()/*boost::filesystem::absolute(sharedModelFilePath(name))*/;
  ::system( ("iscad "+fp.string()+" &").c_str() );
}




void ModelFeature::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "loadmodel",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

     '(' >>
      ( 
	( ruleset.r_identifier >>
	  *(',' >> (ruleset.r_identifier >> '=' >> (ruleset.r_solidmodel_expression|ruleset.r_datumExpression|ruleset.r_vectorExpression|ruleset.r_scalarExpression) ) ) >> ')' )  
	[ qi::_val = phx::bind(&ModelFeature::create, qi::_1, qi::_2) ]
	|
	( ruleset.r_path >> 
	  *(',' >> (ruleset.r_identifier >> '=' >> (ruleset.r_solidmodel_expression|ruleset.r_datumExpression|ruleset.r_vectorExpression|ruleset.r_scalarExpression) ) ) >> ')' )  
	[ qi::_val = phx::bind(&ModelFeature::create_file, qi::_1, qi::_2) ]
      )
    ))
  );
}




FeatureCmdInfoList ModelFeature::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "loadmodel",
            "( <identifier:modelname> [, <identifier> = <feature>|<datum>|<vector>|<scalar> ] )",
            "Imports a submodel. It is read from the file modelname.iscad."
            " The file is searched first in the directory of the current model and then throughout the shared file search path."
            " An arbitrary number of parameters are passed from the current model into the submodel."
        )
    );
}



}
}
