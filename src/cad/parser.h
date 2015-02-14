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

#include "solidmodel.h"
#include "datum.h"
#include "sketch.h"
#include "freeship_interface.h"

#include "base/boost_include.h"

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

namespace insight {
namespace cad {
namespace parser {

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


typedef double scalar;
typedef arma::mat vector;
typedef Datum::Ptr datum;
typedef SolidModel::Ptr solidmodel;
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
  
struct Model
{
  typedef boost::shared_ptr<Model> Ptr;
  
  Model(const ModelSymbols& syms = ModelSymbols());
  
  typedef qi::symbols<char, scalar> scalarSymbolTable;
  scalarSymbolTable scalarSymbols;
  struct vectorSymbolTable : public qi::symbols<char, vector> {} vectorSymbols;
  struct datumSymbolTable : public qi::symbols<char, datum> {} datumSymbols;
  typedef qi::symbols<char, solidmodel> modelstepSymbolTable;
  modelstepSymbolTable modelstepSymbols;
//   std::map<std::string, SolidModel::Ptr> modelstepSymbols;

  struct edgeFeaturesSymbolTable : public qi::symbols<char, FeatureSetPtr> {} edgeFeatureSymbols;

  struct modelSymbolTable : public qi::symbols<char, Model::Ptr> {} modelSymbols;
};

Model::Ptr loadModel(const std::string& name, const ModelSymbols& syms);
BOOST_PHOENIX_ADAPT_FUNCTION(Model::Ptr, loadModel_, loadModel, 2);


}

bool parseISCADModelStream(std::istream& in, parser::Model::Ptr& m, int* failloc=NULL);
bool parseISCADModelFile(const boost::filesystem::path& fn, parser::Model::Ptr& m);


}
}

#undef BOOST_SPIRIT_DEBUG

#endif // INSIGHT_CAD_PARSER_H
