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
#include "base/exception.h"
#include "boost/phoenix/stl/algorithm/transformation.hpp"
#include <boost/phoenix/stl/algorithm/iteration.hpp>
#include "cadfeature.h"
#include "cadmodel.h"
#include "datum.h"
#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "cadexception.h"
#include <memory>

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



void ModelFeature::copyModelDatums(ModelPtr model_)
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

  if (auto* mn = boost::get<std::string>(&modelinput_))
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
  else if (auto* mp = boost::get<boost::filesystem::path>(&modelinput_))
    {
      p+=(*mp);
    }
  else if (auto* mp = boost::get<ModelPtr>(&modelinput_))
    {
        p+=size_t(mp->get());  //(*mp)->hash();
    }

  for (auto& nv: vars_)
  {
    p+=boost::fusion::at_c<0>(nv);

    auto& v=boost::fusion::at_c<1>(nv);
    if (auto* fp = boost::get<FeaturePtr>(&v))
    {
        p+=**fp;
    }
    else if (auto* dp = boost::get<DatumPtr>(&v))
    {
        p+=**dp;
    }
    else if (auto* vp = boost::get<VectorPtrAndType>(&v))
    {
        p+=boost::fusion::at_c<0>(*vp)->value();
        p+=int(boost::fusion::at_c<1>(*vp));
    }
    else if (auto* sp = boost::get<ScalarPtr>(&v))
    {
        p+=(*sp)->value();
    }
  }

  return p.getHash();
}




ModelFeature::ModelFeature ( const ModelFeature& o, TreeCloneMap& tcm )
{
    if (auto *mp=boost::get<ModelPtr>(&o.modelinput_))
    {
        modelinput_=tcm.clone(*mp);
    }
    else
    {
        modelinput_=o.modelinput_;
    }

    struct ModelVariableCloner
        : public boost::static_visitor<ModelVariable>
    {
        TreeCloneMap& tcm_;

        ModelVariableCloner(TreeCloneMap& tcm) : tcm_(tcm) {};

        ModelVariable operator()(FeaturePtr ft) const
        {
            return tcm_.clone(ft);
        }
        ModelVariable operator()(DatumPtr d) const
        {
            return tcm_.clone(d);
        }
        ModelVariable operator()(VectorPtrAndType vt) const
        {
            return VectorPtrAndType{
                tcm_.clone(boost::fusion::get<0>(vt)),
                boost::fusion::get<1>(vt) };
        }
        ModelVariable operator()(ScalarPtr s) const
        {
            return tcm_.clone(s);
        }
    };

    ModelVariableCloner mvc(tcm);
    for (auto v: o.vars_)
    {
        vars_.push_back(ModelVariableAndName{
            boost::fusion::get<0>(v),
            boost::fusion::get<1>(v).apply_visitor(mvc)
        });
    }
}


ModelFeature::ModelFeature(const std::string& modelname, const ModelVariableTable& vars)
: modelinput_(modelname), vars_(vars)
{}




ModelFeature::ModelFeature(const boost::filesystem::path& modelfile, const ModelVariableTable& vars)
: modelinput_(modelfile), vars_(vars)
{}




ModelFeature::ModelFeature(ModelPtr model, const ModelVariableTable& vars)
: modelinput_(model), vars_(vars)
{}




void ModelFeature::build()
{
    insight::CurrentExceptionContext ex(_("building model %s"), featureSymbolName().c_str());
    ExecTimer t("ModelFeature::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        ModelPtr model;

        if (auto* fp = boost::get<boost::filesystem::path>(&modelinput_))
        {
            ex += " from file \""+fp->string()+"\"";
            model.reset(new Model(*fp, vars_));
        }
        else if (auto* mn = boost::get<std::string>(&modelinput_))
        {
            ex += " named "+*mn;
            model.reset(new Model(*mn, vars_));
        }
        else if (auto* mn = boost::get<ModelPtr>(&modelinput_))
        {
            ex += " supplied model";
            model=*mn;
        }
        else
        {
            throw insight::Exception(_("ModelFeature: Model input unspecified!"));
        }
        model->checkForBuildDuringAccess();    

        for (auto& c: model->components())
        {
            components_[c]=model->lookupModelstep(c);
        }

        auto modelsteps=model->modelsteps();
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


        copyModelDatums(model);

        Compound::build();

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<ModelFeature>(hash()));
    }
}


void ModelFeature::replaceDependency(const DependencyReplacement& repl)
{
    for (auto& nv: vars_)
    {
        auto &v = boost::fusion::get<1>(nv);

        if (auto* fp=boost::get<FeaturePtr>(&v))
        {
            repl(*fp);
        }
        else if (auto* fp=boost::get<DatumPtr>(&v))
        {
            repl(*fp);
        }
        else if (auto* fp=boost::get<VectorPtrAndType>(&v))
        {
            repl(boost::fusion::get<0>(*fp));
        }
        else if (auto* fp=boost::get<ScalarPtr>(&v))
        {
            repl(*fp);
        }

    }
    if (auto *mp=boost::get<ModelPtr>(&modelinput_))
    {
        repl(*mp);
    }

    invalidate();
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
//     throw insight::UnhandledSelection();
// }

void ModelFeature::executeEditor()
{
//   std::string name=modelname_+".iscad";
  boost::filesystem::path fp = modelfile()/*boost::filesystem::absolute(sharedModelFilePath(name))*/;
  ::system( ("iscad "+fp.string()+" &").c_str() );
}

ModelVariable *ModelFeature::findInputVariable(const std::string &name)
{
    for (auto& v: vars_)
    {
        if (boost::fusion::get<0>(v)==name)
            return &boost::fusion::get<1>(v);
    }
    return nullptr;
}


FeaturePtr ModelFeature::findModelstep(const std::string &name)
{

    //     //return subshape(name); // will crash

    if (auto* fp=boost::get<ModelPtr>(&modelinput_))
    {
        if (auto *ms = (*fp)->modelstepSymbols().find(name))
        {
            return *ms;
        }
    }
    return nullptr;
}









const ModelFeature::ModelFeatureArgRule&
ModelFeature::rule(
    parser::ISCADParser& ruleset )
{
    const std::string rn="sub model name and variable overrides";

    if (auto r=ruleset.findAdditionalRule<ModelFeatureArgRule>(rn))
    {
        return *r;
    }
    else
    {

        auto &r_modelvar = ruleset.addAdditionalRule(
            new qi::rule<
                std::string::iterator,
                ModelVariable(),
                insight::cad::parser::skip_grammar
                >
            (
                // order of rules must match order of definitions in "typedef ... ModelVariable"
                 ( ruleset.r_vectorExpression > qi::attr(VectorVariableType::Point) )
                |( '!' > ruleset.r_vectorExpression > qi::attr(VectorVariableType::Direction) )
                |( ruleset.r_scalarExpression)
                |( ruleset.r_solidmodel_expression)
                |( ruleset.r_datumExpression)
            ));
        r_modelvar.name("variable value");


        auto &r_modelvars = ruleset.addAdditionalRule(
            new qi::rule<
                std::string::iterator,
                ModelVariableTable(),
                insight::cad::parser::skip_grammar
                >
            (
                *(',' > ruleset.r_identifier > '=' > r_modelvar )
            ));
        r_modelvars.name("model variable list");



        auto &myrule =ruleset.addAdditionalRule(
            new qi::rule<
                std::string::iterator,
                std::shared_ptr<ModelFeature>(ModelVariableTable),
                insight::cad::parser::skip_grammar
                >
            (

                ( ruleset.r_identifier > r_modelvars )
                    [ qi::_val = phx::bind(
                         & ModelFeature::create<const std::string&, const ModelVariableTable&>,
                         qi::_1, phx::bind(&mergeMVTs, qi::_r1, qi::_2) ) ]
                |
                ( ruleset.r_path > r_modelvars )
                    [ qi::_val = phx::bind(
                         &ModelFeature::create<const boost::filesystem::path&, const ModelVariableTable&>,
                         qi::_1, phx::bind(&mergeMVTs, qi::_r1, qi::_2) ) ]
            ));
        myrule.name(rn);

        return myrule;
    }
}



void ModelFeature::insertrule(parser::ISCADParser& ruleset)
{
    typename parser::ISCADParser::ModelstepRulePtr rule(
        new typename parser::ISCADParser::ModelstepRule(
            ( '('
                > ModelFeature::rule(ruleset)
                    ( phx::val(ModelVariableTable()) )
                > ')' )
             [ qi::_val = qi::_1 ]
        ));
    rule->name("loadmodel");

  ruleset.modelstepFunctionRules.add( "loadmodel", rule );
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
