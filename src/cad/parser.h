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

#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"
#include "drawingexport.h"

#ifndef Q_MOC_RUN
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
#endif

#include "cadtypes.h"
#include "cadmodel.h"


namespace insight {
namespace cad {
namespace parser {

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;


typedef insight::cad::ScalarPtr scalar;
typedef insight::cad::VectorPtr vector;
typedef insight::cad::DatumPtr datum;
typedef insight::cad::FeaturePtr solidmodel;
typedef std::pair<std::string, solidmodel > modelstep;
typedef std::vector<modelstep> model;


boost::filesystem::path sharedModelFilePath(const std::string& name);

// double dot(const vector& v1, const vector& v2);
// BOOST_PHOENIX_ADAPT_FUNCTION(vector, vec3_, vec3, 3);
// BOOST_PHOENIX_ADAPT_FUNCTION(vector, cross_, cross, 2);
// BOOST_PHOENIX_ADAPT_FUNCTION(vector, trans_, arma::trans, 1);
// BOOST_PHOENIX_ADAPT_FUNCTION(double, dot_, dot, 2);
// 
// FeatureSetPtr queryVertices(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryVertices_, queryVertices, 3);
// FeatureSetPtr queryAllVertices(const SolidModel& m);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryAllVertices_, queryAllVertices, 1);
// FeatureSetPtr queryVerticesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryVerticesSubset_, queryVerticesSubset, 3);
// 
// FeatureSetPtr queryEdges(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryEdges_, queryEdges, 3);
// FeatureSetPtr queryAllEdges(const SolidModel& m);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryAllEdges_, queryAllEdges, 1);
// FeatureSetPtr queryEdgesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryEdgesSubset_, queryEdgesSubset, 3);
// 
// FeatureSetPtr queryFaces(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryFaces_, queryFaces, 3);
// FeatureSetPtr queryAllFaces(const SolidModel& m);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryAllFaces_, queryAllFaces, 1);
// FeatureSetPtr queryFacesSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryFacesSubset_, queryFacesSubset, 3);
// 
// FeatureSetPtr querySolids(const SolidModel& m, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, querySolids_, querySolids, 3);
// FeatureSetPtr queryAllSolids(const SolidModel& m);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, queryAllSolids_, queryAllSolids, 1);
// FeatureSetPtr querySolidsSubset(const FeatureSetPtr& fs, const std::string& filterexpr, const FeatureSetParserArgList& of);
// BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, querySolidsSubset_, querySolidsSubset, 3);
// 
// 
// 
// 
// Model::Ptr loadModel(const std::string& name, const ModelSymbols& syms);
// BOOST_PHOENIX_ADAPT_FUNCTION(Model::Ptr, loadModel_, loadModel, 2);



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
    typedef qi::rule<std::string::iterator, FeaturePtr(), skip_grammar> ModelstepRule;
    typedef boost::shared_ptr<ModelstepRule> ModelstepRulePtr;

    Model* model_;

    qi::rule<std::string::iterator, ScalarPtr(), skip_grammar> r_scalar_primary, r_scalar_term, r_scalarExpression;
    qi::rule<std::string::iterator, VectorPtr(), qi::locals<FeaturePtr>, skip_grammar > r_vector_primary, r_vector_term, r_vectorExpression;
    
    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr> > r_vertexFeaturesExpression;
    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr> > r_edgeFeaturesExpression;
    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr> > r_faceFeaturesExpression;
    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr> > r_solidFeaturesExpression;
    qi::rule<std::string::iterator, DatumPtr(), skip_grammar> r_datumExpression;
    
    qi::rule<std::string::iterator, skip_grammar> r_model;
    qi::rule<std::string::iterator, skip_grammar> r_assignment;
    qi::rule<std::string::iterator, qi::locals<FeaturePtr>, skip_grammar> r_solidmodel_propertyAssignment;
    qi::rule<std::string::iterator, skip_grammar> r_postproc;
    qi::rule<std::string::iterator, DrawingViewDefinition(), skip_grammar> r_viewDef;
    qi::symbols<char, ModelstepRulePtr> modelstepFunctionRules;
    qi::rule<std::string::iterator, FeaturePtr(), skip_grammar, qi::locals<ModelstepRulePtr> > r_modelstepFunction;
    ModelstepRule r_modelstep;
    qi::rule<std::string::iterator, std::string()> r_identifier;
    qi::rule<std::string::iterator, std::string()> r_string;
    qi::rule<std::string::iterator, boost::filesystem::path()> r_path;
    qi::rule<std::string::iterator, FeaturePtr(), skip_grammar> r_solidmodel_primary, r_solidmodel_term, r_solidmodel_expression;
    qi::rule<std::string::iterator, FeaturePtr(), qi::locals<FeaturePtr>, skip_grammar> r_solidmodel_subshape;
    qi::rule<std::string::iterator, FeaturePtr(), qi::locals<ModelPtr>, skip_grammar> r_submodel_modelstep;
    

    ISCADParser(Model* model);
};

}

bool parseISCADModelStream(std::istream& in, Model* m, int* failloc=NULL);
bool parseISCADModelFile(const boost::filesystem::path& fn, Model* m);


}
}

#undef BOOST_SPIRIT_DEBUG

#endif // INSIGHT_CAD_PARSER_H
