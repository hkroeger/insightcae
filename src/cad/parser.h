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
 *
 */

#ifndef INSIGHT_CAD_PARSER_H
#define INSIGHT_CAD_PARSER_H

#define BOOST_SPIRIT_USE_PHOENIX_V3

#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"

#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/utility/enable_if.hpp>

#include "cadtypes.h"

namespace mapkey_parser
{
  BOOST_SPIRIT_TERMINAL(mapkey);
  
//   namespace tag { struct mapkey{}; } // tag identifying placeholder
//   typedef unspecified<tag::mapkey> mapkey_type;
//   mapkey_type const mapkey = {};   // placeholder itself
}

namespace boost { namespace spirit
{
    // We want custom_parser::iter_pos to be usable as a terminal only,
    // and only for parser expressions (qi::domain).
    template <>
    struct use_terminal<qi::domain, mapkey_parser::tag::mapkey>
      : mpl::true_
    {};
}}

namespace mapkey_parser
{
    template<class T>
    struct mapkey_parser
      : boost::spirit::qi::primitive_parser<mapkey_parser<T> >
    {
	
	const std::map<std::string, T>* map_;
	
	mapkey_parser()
	: map_(NULL)
	{}
	
	mapkey_parser(const std::map<std::string, T>& map)
	: map_(&map)
	{}
	
        // Define the attribute type exposed by this parser component
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef std::string type;
        };
 
        // This function is called during the actual parsing process
        template <typename Iterator, typename Context, typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last, Context&, Skipper const& skipper, Attribute& attr) const
        {
	  if (!map_)
	    throw insight::Exception("Attempt to use unallocated map parser wrapper!");
	  
            boost::spirit::qi::skip_over(first, last, skipper);
	    
	    Iterator cur=first, match=first;
	    bool matched=false;
// 	    cur++;
	    while (cur!=last)
	    {
	      std::string key(first, cur);

	      typename std::map<std::string, T>::const_iterator it=map_->find(key);
	      if (it!=map_->end())
	      {
//   		  std::cout<<"MATCH=>"<<key<<"<"<<std::endl;
		  match=cur;
		  matched=true;
	      }
//  	      else std::cout<<"NOK=>"<<key<<"<"<<std::endl;
	      cur++;
	    }
	    
	    if (matched)
	    {
	      std::string key(first, match);
	      boost::spirit::traits::assign_to(std::string(first, match), attr);
	      first=match;
//  	      std::cout<<"OK! >"<<key<<"<"<<std::endl;
	      return true;
	    }
	    else
	    {
// 	      std::cout<<"NOK!"<<std::endl;
	      return false;
	    }
        }
        
 
        // This function is called during error handling to create
        // a human readable string for the error context.
        template <typename Context>
        boost::spirit::info what(Context&) const
        {
            return boost::spirit::info("mapkey");
        }
    };
}


namespace insight {
namespace cad {
namespace parser {

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


typedef double scalar;
typedef arma::mat vector;
typedef insight::cad::DatumPtr datum;
typedef insight::cad::SolidModelPtr solidmodel;
typedef std::pair<std::string, solidmodel > modelstep;
typedef std::vector<modelstep> model;

typedef boost::tuple<std::string, vector, vector, boost::optional<bool> > viewdef;


double dot(const vector& v1, const vector& v2);
BOOST_PHOENIX_ADAPT_FUNCTION(vector, vec3_, vec3, 3);
BOOST_PHOENIX_ADAPT_FUNCTION(vector, cross_, cross, 2);
BOOST_PHOENIX_ADAPT_FUNCTION(vector, trans_, arma::trans, 1);
BOOST_PHOENIX_ADAPT_FUNCTION(double, dot_, dot, 2);

void writeViews(const boost::filesystem::path& file, const solidmodel& model, const std::vector<viewdef>& viewdefs);
BOOST_PHOENIX_ADAPT_FUNCTION(void, writeViews_, writeViews, 3);

FeatureSetPtr queryEdges(const SolidModel& m, const std::string& filterexpr, const FeatureSetList& of);
BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryEdges_, queryEdges, 3);

typedef boost::variant<scalar, vector>  ModelSymbol;
typedef std::vector<boost::fusion::vector2<std::string, ModelSymbol> > ModelSymbols;


class Model
{
public:
  typedef boost::shared_ptr<Model> Ptr;
  
  typedef std::map<std::string, scalar> 	scalarSymbolTable;
  typedef std::map<std::string, vector> 	vectorSymbolTable;
  typedef std::map<std::string, datum> 		datumSymbolTable;
  typedef std::map<std::string, solidmodel> 	modelstepSymbolTable;
  typedef std::map<std::string, FeatureSetPtr> 	edgeFeatureSymbolTable;
  typedef std::map<std::string, Model::Ptr> 	modelSymbolTable;
  typedef std::map<std::string, EvaluationPtr> 	evaluationSymbolTable;
  
  typedef std::set<std::string> componentTable;
  
protected:
  gp_Ax3 placement_;
  scalarSymbolTable 		scalarSymbols_;
  vectorSymbolTable 		vectorSymbols_;
  datumSymbolTable 		datumSymbols_;
  modelstepSymbolTable		modelstepSymbols_;
  edgeFeatureSymbolTable	edgeFeatureSymbols_;
  modelSymbolTable		modelSymbols_;
  evaluationSymbolTable		evaluationSymbols_;
  componentTable		components_;
  
public:
  
  Model(const ModelSymbols& syms = ModelSymbols());

  mapkey_parser::mapkey_parser<scalar> scalarSymbolNames() const;
  mapkey_parser::mapkey_parser<vector> vectorSymbolNames() const;
  mapkey_parser::mapkey_parser<datum> datumSymbolNames() const;
  mapkey_parser::mapkey_parser<solidmodel> modelstepSymbolNames() const;
  mapkey_parser::mapkey_parser<FeatureSetPtr> edgeFeatureSymbolNames() const;
  mapkey_parser::mapkey_parser<Model::Ptr> modelSymbolNames() const;
  mapkey_parser::mapkey_parser<EvaluationPtr> evaluationSymbolNames() const;

    
  inline void addScalarSymbol(const std::string& name, const scalar& value)
  {
    scalarSymbols_[name]=value;
  }
  inline void addScalarSymbolIfNotPresent(const std::string& name, const scalar& value)
  {
    if (scalarSymbols_.find(name)==scalarSymbols_.end())
      scalarSymbols_[name]=value;
  }
  inline void addVectorSymbol(const std::string& name, const vector& value)
  {
    vectorSymbols_[name]=value;
  }
  inline void addVectorSymbolIfNotPresent(const std::string& name, const vector& value)
  {
    if (vectorSymbols_.find(name)==vectorSymbols_.end())
      vectorSymbols_[name]=value;
  }
  inline void addDatumSymbol(const std::string& name, const datum& value)
  {
    datumSymbols_[name]=value;
  }
  inline void addModelstepSymbol(const std::string& name, const solidmodel& value)
  {
    modelstepSymbols_[name]=value;
  }
  inline void addComponent(const std::string& name, const solidmodel& value)
  {
    addModelstepSymbol(name, value);
    components_.insert(name);
  }
  inline void addEdgeFeatureSymbol(const std::string& name, const FeatureSetPtr& value)
  {
    edgeFeatureSymbols_[name]=value;
  }
  inline void addModelSymbol(const std::string& name, const Model::Ptr& value)
  {
    modelSymbols_[name]=value;
  }
  inline void addEvaluationSymbol(const std::string& name, const EvaluationPtr& value)
  {
    evaluationSymbols_[name]=value;
  }
  
  inline scalar lookupScalarSymbol(const std::string& name) const
  {
    scalarSymbolTable::const_iterator it=scalarSymbols_.find(name);
    if (it==scalarSymbols_.end())
      throw insight::Exception("Could not lookup scalar symbol "+name);
    return it->second;
  }
  inline vector lookupVectorSymbol(const std::string& name) const
  {
    vectorSymbolTable::const_iterator it=vectorSymbols_.find(name);
    if (it==vectorSymbols_.end())
      throw insight::Exception("Could not lookup vector symbol "+name);
    return it->second;
  }
  inline datum lookupDatumSymbol(const std::string& name) const
  {
    datumSymbolTable::const_iterator it=datumSymbols_.find(name);
    if (it==datumSymbols_.end())
      throw insight::Exception("Could not lookup datum symbol "+name);
    return it->second;
  }
  inline solidmodel lookupModelstepSymbol(const std::string& name) const
  {
    modelstepSymbolTable::const_iterator it=modelstepSymbols_.find(name);
    if (it==modelstepSymbols_.end())
      throw insight::Exception("Could not lookup model step symbol "+name);
    return it->second;
  }
  inline FeatureSetPtr lookupEdgeFeatureSymbol(const std::string& name) const
  {
    edgeFeatureSymbolTable::const_iterator it=edgeFeatureSymbols_.find(name);
    if (it==edgeFeatureSymbols_.end())
      throw insight::Exception("Could not lookup edge feature symbol "+name);
    return it->second;
  }
  inline Model::Ptr lookupModelSymbol(const std::string& name) const
  {
    modelSymbolTable::const_iterator it=modelSymbols_.find(name);
    if (it==modelSymbols_.end())
      throw insight::Exception("Could not lookup model symbol "+name);
    return it->second;
  }
  inline EvaluationPtr lookupEvaluationSymbol(const std::string& name) const
  {
    evaluationSymbolTable::const_iterator it=evaluationSymbols_.find(name);
    if (it==evaluationSymbols_.end())
      throw insight::Exception("Could not lookup evaluation symbol "+name);
    return it->second;
  }
  
  const std::map<std::string, scalar>& scalarSymbols() const { return scalarSymbols_; }
  const std::map<std::string, vector>& vectorSymbols() const { return vectorSymbols_; }
  const std::map<std::string, datum>& datumSymbols() const { return datumSymbols_; }
  const std::map<std::string, solidmodel>& modelstepSymbols() const { return modelstepSymbols_; }  
  const std::map<std::string, FeatureSetPtr>& edgeFeatureSymbols() const { return edgeFeatureSymbols_; }  
  const std::map<std::string, Model::Ptr>& modelSymbols() const { return modelSymbols_; }  
  const std::map<std::string, EvaluationPtr>& evaluationSymbols() const { return evaluationSymbols_; }  
  
//   struct vectorSymbolTable : public qi::symbols<char, vector> {} vectorSymbols;
//   struct datumSymbolTable : public qi::symbols<char, datum> {} datumSymbols;
//   typedef qi::symbols<char, solidmodel> modelstepSymbolTable;
//   modelstepSymbolTable modelstepSymbols;
// //   std::map<std::string, SolidModelPtr> modelstepSymbols;
// 
//   struct edgeFeaturesSymbolTable : public qi::symbols<char, FeatureSetPtr> {} edgeFeatureSymbols;
// 
//   struct modelSymbolTable : public qi::symbols<char, Model::Ptr> {} modelSymbols;

  arma::mat modelCoG();
//   inline arma::mat modelCoG() { return modelCoG(); }; // "const" caused failure with phx::bind!
  
  /**
   * return bounding box of model
   * first col: min point
   * second col: max point
   */
  arma::mat modelBndBox(double deflection=-1) const;

};

Model::Ptr loadModel(const std::string& name, const ModelSymbols& syms);
BOOST_PHOENIX_ADAPT_FUNCTION(Model::Ptr, loadModel_, loadModel, 2);


class SoidModelPtr;


// template <typename Iterator>
struct skip_grammar 
: public qi::grammar<std::string::iterator>
{
  qi::rule<std::string::iterator> skip;
  skip_grammar();
};

struct ISCADParser
  : qi::grammar<std::string::iterator, skip_grammar>
{
    typedef qi::rule<std::string::iterator, solidmodel(), skip_grammar> ModelstepRule;
    typedef boost::shared_ptr<ModelstepRule> ModelstepRulePtr;

    Model::Ptr model_;

    qi::rule<std::string::iterator, scalar(), skip_grammar> r_scalar_primary, r_scalar_term, r_scalarExpression;
    qi::rule<std::string::iterator, vector(), skip_grammar> r_vector_primary, r_vector_term, r_vectorExpression;
    
    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar> r_edgeFeaturesExpression;
    qi::rule<std::string::iterator, datum(), skip_grammar> r_datumExpression;
    
    qi::rule<std::string::iterator, skip_grammar> r_model;
    qi::rule<std::string::iterator, skip_grammar> r_assignment;
    qi::rule<std::string::iterator, qi::locals<SolidModelPtr>, skip_grammar> r_solidmodel_propertyAssignment;
    qi::rule<std::string::iterator, skip_grammar> r_postproc;
    qi::rule<std::string::iterator, viewdef(), skip_grammar> r_viewDef;
    qi::symbols<char, ModelstepRulePtr> modelstepFunctionRules;
    qi::rule<std::string::iterator, solidmodel(), skip_grammar, qi::locals<ModelstepRulePtr> > r_modelstepFunction;
    ModelstepRule r_modelstep;
    qi::rule<std::string::iterator, std::string()> r_identifier;
    qi::rule<std::string::iterator, std::string()> r_string;
    qi::rule<std::string::iterator, boost::filesystem::path()> r_path;
    qi::rule<std::string::iterator, solidmodel(), skip_grammar> r_solidmodel_primary, r_solidmodel_term, r_solidmodel_expression;
    qi::rule<std::string::iterator, solidmodel(), qi::locals<SolidModelPtr>, skip_grammar> r_solidmodel_subshape;
    qi::rule<std::string::iterator, solidmodel(), qi::locals<Model::Ptr>, skip_grammar> r_submodel_modelstep;
    

    ISCADParser(Model::Ptr model);
};

}

bool parseISCADModelStream(std::istream& in, parser::Model::Ptr& m, int* failloc=NULL);
bool parseISCADModelFile(const boost::filesystem::path& fn, parser::Model::Ptr& m);


}
}

#undef BOOST_SPIRIT_DEBUG

#endif // INSIGHT_CAD_PARSER_H
