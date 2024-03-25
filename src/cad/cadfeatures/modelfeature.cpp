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
#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "base/translations.h"

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
//addToFactoryTable(Feature, ModelFeature);
addToStaticFunctionTable(Feature, ModelFeature, insertrule);
addToStaticFunctionTable(Feature, ModelFeature, ruleDocumentation);



void ModelFeature::copyModelDatums()
{
  auto scalars=model_->scalars();
  for (decltype(scalars)::value_type const& v: scalars)
  {
    if (refvalues_.find(v.first)!=refvalues_.end())
          throw insight::Exception(_("datum value %s already present!"), v.first.c_str());
    refvalues_[v.first]=v.second->value();
  }

  auto points=model_->points();
  for (decltype(points)::value_type const& p: points)
  {
    if (refpoints_.find(p.first)!=refpoints_.end())
          throw insight::Exception(_("datum point %s already present!"), p.first.c_str());
    refpoints_[p.first]=p.second->value();
  }

  auto directions=model_->directions();
  for (decltype(directions)::value_type const& p: directions)
  {
    if (refvectors_.find(p.first)!=refvectors_.end())
          throw insight::Exception(_("datum direction %s already present!"), p.first.c_str());
    refvectors_[p.first]=p.second->value();
  }

  auto datums=model_->datums();
  for (decltype(datums)::value_type const& d: datums)
  {
    if (providedDatums_.find(d.first)!=providedDatums_.end())
          throw insight::Exception(_("datum %s already present!"), d.first.c_str());

    providedDatums_[d.first]=d.second;
  }

  auto addfs=
      [this](const std::string& name, FeatureSetPtr fs)
  {
      providedFeatureSets_[name]=fs;
  };
  model_->vertexFeatureSymbols().for_each(addfs);
  model_->edgeFeatureSymbols().for_each(addfs);
  model_->faceFeatureSymbols().for_each(addfs);
  model_->solidFeatureSymbols().for_each(addfs);

}



size_t ModelFeature::calcHash() const
{
  // build the parameter hash
  ParameterListHash p;
  p+=this->type();
  if (const std::string* mn = boost::get<std::string>(&modelinput_))
    {
      std::string fname=(*mn)+".iscad";
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
    }
  else if (const boost::filesystem::path* mp = boost::get<boost::filesystem::path>(&modelinput_))
    {
      p+=(*mp);
    }

  for (ModelVariableTable::const_iterator it=vars_.begin(); it!=vars_.end(); it++)
  {
    p+=boost::fusion::at_c<0>(*it);
    auto v=boost::fusion::at_c<1>(*it);
    if (FeaturePtr* fp = boost::get<FeaturePtr>(&v))
    {
        p+=*(*fp);
    }
    else if (DatumPtr* dp = boost::get<DatumPtr>(&v))
    {
        p+=*(*dp);
    }
    else if (VectorPtrAndType* vp = boost::get<VectorPtrAndType>(&v))
    {
        p+=boost::fusion::at_c<0>(*vp)->value();
        p+=int(boost::fusion::at_c<1>(*vp));
    }
    else if (ScalarPtr* sp = boost::get<ScalarPtr>(&v))
    {
        p+=(*sp)->value();
    }
  }

  return p.getHash();
}





ModelFeature::ModelFeature(const std::string& modelname, const ModelVariableTable& vars)
: modelinput_(modelname), vars_(vars)
{}




ModelFeature::ModelFeature(const boost::filesystem::path& modelfile, const ModelVariableTable& vars)
: modelinput_(modelfile), vars_(vars)
{}




ModelFeature::ModelFeature(ModelPtr model)
: model_(model)
{}




void ModelFeature::build()
{
    insight::CurrentExceptionContext ex(_("building model %s"), featureSymbolName().c_str());
    ExecTimer t("ModelFeature::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        if (!model_)
        {
          if (boost::filesystem::path* fp = boost::get<boost::filesystem::path>(&modelinput_))
          {
            ex += " from file \""+fp->string()+"\"";
            model_.reset(new Model(*fp, vars_));
          } else if (std::string* mn = boost::get<std::string>(&modelinput_))
          {
            ex += " named "+*mn;
            model_.reset(new Model(*mn, vars_));
          } else
          {
            throw insight::Exception(_("ModelFeature: Model input unspecified!"));
          }
        }

        model_->checkForBuildDuringAccess();

        for (const Model::ComponentSet::value_type& c: model_->components())
        {
            components_[c]=model_->lookupModelstep(c);
        }

        auto modelsteps=model_->modelsteps();
        for (decltype(modelsteps)::value_type const& c: modelsteps)
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
    if (const boost::filesystem::path* fp = boost::get<boost::filesystem::path>(&modelinput_))
    {
      std::string fname = boost::filesystem::basename(*fp);
      boost::erase_last(fname, ".iscad");
      return fname;
    } 
    else if (const std::string* mn = boost::get<std::string>(&modelinput_))
    {
      return *mn;
    } 
    else
    {
      throw insight::Exception(_("ModelFeature: Model input unspecified!"));
      return std::string();
    }
}


boost::filesystem::path ModelFeature::modelfile() const
{
    if (const boost::filesystem::path* fp = boost::get<boost::filesystem::path>(&modelinput_))
    {
      return boost::filesystem::absolute(*fp);
    } 
    else if (const std::string* mn = boost::get<std::string>(&modelinput_))
    {
      return boost::filesystem::absolute(sharedModelFilePath(*mn+".iscad"));
    } 
    else
    {
      throw insight::Exception(_("ModelFeature: Model input unspecified!"));
      return boost::filesystem::path();
    }
}

// boost::spirit::qi::symbols<char, FeatureSetPtr>
// ModelFeature::featureSymbols(EntityType et) const
// {
//     switch (et)
//     {
//     case Vertex:
//         return model_->vertexFeatureSymbols();
//     case Edge:
//         return model_->edgeFeatureSymbols();
//     case Face:
//         return model_->faceFeatureSymbols();
//     case Solid:
//         return model_->solidFeatureSymbols();
//     }
//     throw insight::Exception("internal error: unhandled selection!");
// }

void ModelFeature::executeEditor()
{
//   std::string name=modelname_+".iscad";
  boost::filesystem::path fp = modelfile()/*boost::filesystem::absolute(sharedModelFilePath(name))*/;
  ::system( ("iscad "+fp.string()+" &").c_str() );
}




void ModelFeature::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "loadmodel",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

     '(' >>
      ( 
	( ruleset.r_identifier >>
          *(',' >> ( ruleset.r_identifier >> (
                      ('=' >> ruleset.r_solidmodel_expression)|
                      ('=' >> ruleset.r_datumExpression)|
                      ('=' >> ruleset.r_vectorExpression >> qi::attr(VectorVariableType::Point) )|
                      (qi::lit("!=") >> ruleset.r_vectorExpression >> qi::attr(VectorVariableType::Direction) )|
                      ('=' >> ruleset.r_scalarExpression) ) ) ) >> ')' )
    [ qi::_val = phx::bind(
                           &ModelFeature::create<const std::string&, const ModelVariableTable&>,
                           qi::_1, qi::_2) ]
	|
	( ruleset.r_path >> 
          *(',' >> (ruleset.r_identifier >> (
                      ('=' >> ruleset.r_solidmodel_expression)|
                      ('=' >> ruleset.r_datumExpression)|
                      (qi::lit("!=") >> ruleset.r_vectorExpression >> qi::attr(VectorVariableType::Direction) )|
                      ('=' >> ruleset.r_scalarExpression) ) ) ) >> ')' )
    [ qi::_val = phx::bind(
                           &ModelFeature::create<const boost::filesystem::path&, const ModelVariableTable&>,
                           qi::_1, qi::_2) ]
      )
    ))
  );
}




FeatureCmdInfoList ModelFeature::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "loadmodel",
            "( <identifier:modelname> [, <identifier> = <feature>|<datum>|<vector>|<scalar> ] )",
            _("Imports a submodel. It is read from the file modelname.iscad."
            " The file is searched first in the directory of the current model and then throughout the shared file search path."
            " An arbitrary number of parameters are passed from the current model into the submodel.")
        )
    };
}



}
}
