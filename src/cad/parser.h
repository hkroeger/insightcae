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

#include "base/extendedgrammar.h"
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"
#include "cadpostprocactions/drawingexport.h"

#ifndef Q_MOC_RUN
#include "boost/variant/recursive_variant.hpp"

#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>

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




typedef insight::cad::ScalarPtr scalar;
typedef insight::cad::VectorPtr vector;
typedef insight::cad::DatumPtr datum;
typedef insight::cad::FeaturePtr solidmodel;
typedef std::pair<std::string, solidmodel > modelstep;
typedef std::vector<modelstep> model;

typedef std::pair<long, long> SyntaxElementPos;
typedef std::pair<boost::filesystem::path, SyntaxElementPos> SyntaxElementLocation;

class SyntaxElementDirectory
: public std::map<SyntaxElementLocation, FeaturePtr>
{
public:
    void addEntry(SyntaxElementLocation location, FeaturePtr element);
    FeaturePtr findElement(long location, const boost::filesystem::path& file="") const;
    SyntaxElementLocation findElement(ConstFeaturePtr element) const;
};

typedef std::shared_ptr<SyntaxElementDirectory> SyntaxElementDirectoryPtr;


// template <typename Iterator>
struct skip_grammar 
: public qi::grammar<std::string::iterator>
{
  qi::rule<std::string::iterator> skip;
  skip_grammar();
};

template <typename T>
struct make_shared_f
{
  template <typename... A> struct result
  { typedef std::shared_ptr<T> type; };

  template <typename... A>
  typename result<A...>::type operator()(A&&... a) const
  {
      return std::make_shared<T>(std::forward<A>(a)...);
  }
};

template <typename T>
using make_shared_ = boost::phoenix::function<make_shared_f<T> >;









class iscadParserException
: public Exception
{
    int from_pos_, to_pos_;
public:
   iscadParserException(const std::string& reason, int from_pos, int to_pos);
   
   inline int from_pos() const { return from_pos_; }
   inline int to_pos() const { return to_pos_; }
};




struct ISCADParser
    : insight::ExtendedGrammar<qi::grammar<std::string::iterator, skip_grammar> >
{
    CurrentPos<std::string::iterator> current_pos;
    boost::filesystem::path filenameinfo_;
    SyntaxElementDirectoryPtr syntax_element_locations;

    typedef qi::rule<std::string::iterator, FeaturePtr(), skip_grammar> ModelstepRule;
    typedef std::shared_ptr<ModelstepRule> ModelstepRulePtr;

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
    qi::rule<std::string::iterator, skip_grammar> r_postproc, r_doc;
    qi::rule<std::string::iterator, DrawingViewDefinition(), skip_grammar> r_viewDef;
    qi::symbols<char, ModelstepRulePtr> modelstepFunctionRules;
    qi::rule<std::string::iterator, boost::fusion::vector3<std::size_t, std::size_t, FeaturePtr>(), skip_grammar, qi::locals<ModelstepRulePtr> > r_modelstepFunction;
    ModelstepRule r_modelstep;
    qi::rule<std::string::iterator, boost::fusion::vector3<std::size_t, FeaturePtr, std::size_t>(), skip_grammar > r_modelstepSymbol;
    qi::rule<std::string::iterator, std::string()> r_identifier;
    qi::rule<std::string::iterator, std::string()> r_string;
    qi::rule<std::string::iterator, boost::filesystem::path()> r_path;
    qi::rule<std::string::iterator, FeaturePtr(), skip_grammar> r_solidmodel_primary, r_solidmodel_term, r_solidmodel_expression;


    ISCADParser(Model* model, const boost::filesystem::path& filenameinfo="");
    
    void createScalarExpressions();
    void createVectorExpressions();
    void createPostProcExpressions();
    void createDocExpressions();
    void createFeatureExpressions();
    void createDatumExpressions();
    void createSelectionExpressions();
};

}


bool parseISCADModel
(
    const std::string& script,
    Model* m,
    int* failloc=NULL,
    parser::SyntaxElementDirectoryPtr* sd=NULL,
    const boost::filesystem::path& filenameinfo=""
);


bool parseISCADModelStream
(
    std::istream& in, 
    Model* m, 
    int* failloc=NULL, 
    parser::SyntaxElementDirectoryPtr* sd=NULL,
    const boost::filesystem::path& filenameinfo=""
);

bool parseISCADModelFile
(
    const boost::filesystem::path& fn, 
    Model* m, 
    int* failloc=NULL, 
    parser::SyntaxElementDirectoryPtr* sd=NULL
);


}
}

// #undef BOOST_SPIRIT_DEBUG

#endif // INSIGHT_CAD_PARSER_H
