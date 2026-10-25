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
#include "parser_tokens.h"


namespace insight {
namespace cad {

class ModelFeature;


namespace parser {




typedef insight::cad::ScalarPtr scalar;
typedef insight::cad::VectorPtr vector;
typedef insight::cad::DatumPtr datum;
typedef insight::cad::FeaturePtr solidmodel;
typedef std::pair<std::string, solidmodel > modelstep;
typedef std::vector<modelstep> model;

typedef std::pair<boost::filesystem::path, SyntaxElementPos> SyntaxElementLocation;

std::ostream& operator<<(std::ostream& os, const SyntaxElementLocation& sel);

typedef boost::variant<FeaturePtr,FeatureSetPtr> SyntaxElement;


class SyntaxElementDirectory
: public std::map<SyntaxElementLocation, SyntaxElement>
{
public:
    /**
     * recognized tokens of the main script, keyed by begin position.
     * All positions are byte offsets in the script.
     * A token is replaced, when another one with the same begin position is recorded
     * (the last recording is from the alternative, which finally matched).
     */
    std::map<long, ScriptToken> tokens;

    /**
     * collapsible ranges, keyed by begin position
     */
    std::map<long, FoldRange> foldRanges;

    /**
     * all available command and function names
     */
    std::map<std::string, CommandKind> commands;

    void addEntry(SyntaxElementLocation location, FeaturePtr element);
    void addFSEntry(SyntaxElementLocation location, FeatureSetPtr element);

    void addToken(long begin, long end, TokenKind kind);
    void addFoldRange(long begin, long end, const std::string& command);

    SyntaxElement findElement(long location, const boost::filesystem::path& file="") const;

    template<class T>
    SyntaxElementLocation findLocation(const T& element) const
    {
        if (element)
        {
            const_iterator it = std::find_if
                (
                    this->begin(),
                    this->end(),
                    [&element](const value_type & t) -> bool
                    {
                        if (auto *fp = boost::get<T>(&t.second))
                        {
                            return *fp == element;
                        }
                        else return false;
                    }
                    );
            if (it!=end())
                return it->first;
        }
        return SyntaxElementLocation("", SyntaxElementPos(-1, -1));
    }

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









/**
 * error in an ISCAD script.
 * message() returns a compiler-style formatted message
 * including location and source excerpt.
 */
class iscadParserException
: public Exception
{
    int from_pos_, to_pos_;
    int line_=0, column_=0;
    boost::filesystem::path file_;
    std::string diagnostic_;
    std::vector<std::string> notes_;

public:
   iscadParserException(const std::string& reason, int from_pos, int to_pos);

   /**
    * @param diagnostic the error description without location
    * @param notes additional hints, usage information etc.
    * @param script the parsed script text (for location and excerpt)
    * @param from_pos,to_pos character range of the error, -1 if unknown
    */
   iscadParserException(
       const std::string& diagnostic,
       const std::vector<std::string>& notes,
       const std::string& script,
       const boost::filesystem::path& file,
       int from_pos, int to_pos );

   inline int from_pos() const { return from_pos_; }
   inline int to_pos() const { return to_pos_; }

   /**
    * 1-based line and column, 0 if unknown
    */
   inline int line() const { return line_; }
   inline int column() const { return column_; }

   inline const boost::filesystem::path& file() const { return file_; }
   inline const std::string& diagnostic() const { return diagnostic_; }
   inline const std::vector<std::string>& notes() const { return notes_; }

   /**
    * single line summary without source excerpt
    */
   std::string summary() const;
};


class SubmodelRule;
typedef std::shared_ptr<SubmodelRule> SubmodelRulePtr;



struct ISCADParser
    : insight::ExtendedGrammar<qi::grammar<std::string::iterator, skip_grammar> >
{
    typedef insight::cad::parser::CommandKind CommandKind;

    CurrentPos<std::string::iterator> current_pos;

    /**
     * the current position, without skipping whitespace and comments before.
     * To be used for the end positions of tokens.
     */
    qi::rule<std::string::iterator, std::size_t(), skip_grammar> r_endPos;
    boost::filesystem::path filenameinfo_;
    SyntaxElementDirectoryPtr syntax_element_locations;

    typedef qi::rule<std::string::iterator, FeaturePtr(), skip_grammar> ModelstepRule;
    typedef std::shared_ptr<ModelstepRule> ModelstepRulePtr;
    qi::symbols<char, ModelstepRulePtr> modelstepFunctionRules;
    qi::rule<std::string::iterator,
             boost::fusion::vector3<std::size_t, std::size_t, FeaturePtr>(),
             skip_grammar,
             qi::locals<ModelstepRulePtr> > r_modelstepFunction;


    typedef qi::rule<std::string::iterator, ScalarPtr(), skip_grammar> ScalarFunctionRule;
    typedef std::shared_ptr<ScalarFunctionRule> ScalarFunctionRulePtr;
    qi::symbols<char, ScalarFunctionRulePtr> scalarFunctionRules;
    qi::rule<std::string::iterator,
             boost::fusion::vector3<std::size_t, std::size_t, ScalarPtr>(),
             skip_grammar,
             qi::locals<ScalarFunctionRulePtr> > r_scalarFunction;


    typedef qi::rule<std::string::iterator, VectorPtr(), skip_grammar> VectorFunctionRule;
    typedef std::shared_ptr<VectorFunctionRule> VectorFunctionRulePtr;
    qi::symbols<char, VectorFunctionRulePtr> vectorFunctionRules;
    qi::rule<std::string::iterator,
             boost::fusion::vector3<std::size_t, std::size_t, VectorPtr>(),
             skip_grammar,
             qi::locals<VectorFunctionRulePtr> > r_vectorFunction;



    typedef qi::rule<std::string::iterator, PostprocActionPtr(), skip_grammar> PostProcFunctionRule;
    typedef std::shared_ptr<PostProcFunctionRule> PostProcFunctionRulePtr;
    qi::symbols<char, PostProcFunctionRulePtr> postProcFunctionRules;
    qi::rule<std::string::iterator, skip_grammar,
             qi::locals<PostProcFunctionRulePtr> > r_postproc;

    Model* model_;

    qi::rule<std::string::iterator, std::shared_ptr<DescriptionWithParameters>(), skip_grammar >
        r_descriptionWithParameters;

    qi::rule<std::string::iterator, BOMDescriptionDataPtr(), skip_grammar >
        r_BOMDescriptionData;

    qi::rule<std::string::iterator, ScalarPtr(), skip_grammar, qi::locals<VectorPtr> >
        r_scalar_primary;

    qi::rule<std::string::iterator, ScalarPtr(), skip_grammar>
        r_scalar_term, r_scalarExpression;

    qi::rule<std::string::iterator, VectorPtr(), qi::locals<FeaturePtr>, skip_grammar >
        r_vector_primary, r_vector_term, r_vectorExpression;
    
    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr,std::size_t> >
        r_vertexFeaturesExpression;

    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr,std::size_t> >
        r_edgeFeaturesExpression;

    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr,std::size_t> >
        r_faceFeaturesExpression;

    qi::rule<std::string::iterator, FeatureSetPtr(), skip_grammar, qi::locals<FeaturePtr,std::size_t> >
        r_solidFeaturesExpression;

    qi::rule<std::string::iterator, DatumPtr(), skip_grammar> r_datumExpression;
    
    qi::rule<std::string::iterator, skip_grammar> r_model;
    qi::rule<std::string::iterator, skip_grammar, qi::locals<std::string,SyntaxElementPos> > r_assignment;
    qi::rule<std::string::iterator, qi::locals<FeaturePtr>, skip_grammar> r_solidmodel_propertyAssignment;
    qi::rule<std::string::iterator, skip_grammar> r_doc;
    qi::rule<std::string::iterator, skip_grammar> r_deprecatedPostKeyword;
    qi::rule<std::string::iterator, DrawingViewDefinition(), skip_grammar> r_viewDef;

    // ModelstepRule r_modelstep;
    qi::rule<std::string::iterator, boost::fusion::vector3<std::size_t, FeaturePtr, std::size_t>(), skip_grammar > r_modelstepSymbol;
    qi::rule<std::string::iterator, std::string()> r_identifier;
    qi::rule<std::string::iterator, std::string()> r_string;
    qi::rule<std::string::iterator, boost::filesystem::path()> r_path;
    qi::rule<std::string::iterator, FeaturePtr(), skip_grammar> r_solidmodel_primary, r_solidmodel_term, r_solidmodel_expression;

    qi::rule<std::string::iterator,
             std::shared_ptr<ModelFeature>(ModelVariableTable),
             skip_grammar, qi::locals<SubmodelRulePtr, std::size_t> >
        r_submodel;


    // state for error reporting

    /**
     * character ranges of the names of all commands/functions,
     * whose arguments are currently being parsed (innermost last).
     * Remains intact, if an expectation failure is thrown.
     */
    std::vector<SyntaxElementPos> commandStack_;
    void pushCommand(std::size_t nameBegin, std::size_t nameEnd, CommandKind kind);
    void popCommand();

    /**
     * the argument list of the innermost command ends at argsEnd:
     * record the collapsible range and pop the command
     */
    void endCommand(std::size_t argsEnd);

    /**
     * record a token at the given iterator range
     * (absolute position w.r.t. the start of the main script)
     */
    void addToken(
        std::string::const_iterator begin,
        std::string::const_iterator end,
        TokenKind kind );

    /**
     * record the range of an in-situ submodel block
     */
    void endSubmodel(std::size_t begin, std::size_t end);

    /**
     * record the name of a newly defined symbol
     */
    void defineSymbol(const SyntaxElementPos& namePos, TokenKind kind);

    /**
     * a parser for a symbol from the given table, which records a token of the given kind
     */
    template<class Map>
    std::shared_ptr<qi::rule<std::string::iterator, typename Map::mapped_type(), skip_grammar> >
    recordedLookup(const Map& map, TokenKind kind);

    enum Section { ModelSection, DocSection };
    Section section_ = ModelSection;

    /**
     * locations of the deprecated (and ignored) "@post" keyword
     */
    std::vector<std::size_t> deprecatedPostKeywordPositions_;
    void deprecatedPostKeyword(std::size_t pos);

    /**
     * translated description of a command kind, e.g. "feature command"
     */
    static std::string commandKindDescription(CommandKind kind);

    /**
     * all command and function names with their kind
     */
    std::map<std::string, CommandKind> commandKinds_;


    /**
     * @param sharedSyntaxElementLocations
     * if given, syntax elements are recorded into this directory instead of a new one
     */
    ISCADParser(
        Model* model,
        const boost::filesystem::path& filenameinfo="",
        SyntaxElementDirectoryPtr sharedSyntaxElementLocations = SyntaxElementDirectoryPtr() );
    
    void createScalarExpressions();
    void createVectorExpressions();
    void createPostProcExpressions();
    void createDocExpressions();
    void createFeatureExpressions();
    void createDatumExpressions();
    void createSelectionExpressions();
};




class SubmodelRule
{
    /**
     * the submodel parser records its tokens into the
     * syntax element directory of the parent parser
     * and uses the same position reference.
     */
    SubmodelRule(
        ISCADParser* parentParser,
        const ModelVariableTable& addVars );

public:
    insight::cad::ModelPtr submodel_;
    ISCADParser submodelParser_;

    CREATE_FUNCTION(SubmodelRule);

    const qi::rule<std::string::iterator, skip_grammar>&
    rule() const;
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


/**
 * @brief parseISCADScalarExpression
 * parses a single scalar expression
 * @param expression
 * expression text in ISCAD syntax, e.g. "0.5*L"
 * @param scope
 * symbols, which can be referenced in the expression
 * @return the parsed and evaluated expression. Throws on error.
 */
ScalarPtr parseISCADScalarExpression
(
    const std::string& expression,
    const ModelVariableTable& scope
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
