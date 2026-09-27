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

#include "cadtypes.h"
#include <memory>
#include <cstring>
#ifdef INSIGHT_CAD_DEBUG
#define BOOST_SPIRIT_DEBUG
#endif


#include "cadfeature.h"

#include "datum.h"
#include "sketch.h"
#include "cadpostprocactions.h"

#include "base/analysis.h"
#include "base/tools.h"
#include "parser.h"
#include "base/translations.h"
#include "parser_tools.h"
#include "parser_errors.h"
#include "boost/locale.hpp"
#include "base/boost_include.h"
#include "boost/make_shared.hpp"
#include <boost/fusion/adapted.hpp>
#include <boost/phoenix/fusion.hpp>

#include "cadfeatures.h"
#include "meshing.h"

#include "cadfeatures/modelfeature.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;




// phx::at shall return value reference instead of key/value pair
namespace boost { namespace phoenix { namespace stl {
    template <typename This, typename Key, typename Value, typename Compare, typename Allocator, typename Index>
        struct at_impl::result<This(std::map<Key,Value,Compare,Allocator>&, Index)>
        {
            typedef Value & type;
        };
    template <typename This, typename Key, typename Value, typename Compare, typename Allocator, typename Index>
        struct at_impl::result<This(std::map<Key,Value,Compare,Allocator> const&, Index)>
        {
            typedef Value const& type;
        };
}}}




namespace insight {
namespace cad {


sharedModelLocations::sharedModelLocations()
{
  CurrentExceptionContext ec(_("building list of shared model locations"));

  const char* e=getenv("ISCAD_MODEL_PATH");
  if (e)
  {
    std::vector<std::string> paths;
    boost::split(paths, e, boost::is_any_of(":"));
    std::copy(paths.begin(), paths.end(), back_inserter(*this));
  }
  {
      for (const path& p: insight::SharedPathList::global())
      {
        if (boost::filesystem::is_directory(p/"iscad-library"))
          push_back(p/"iscad-library");
        else if (boost::filesystem::is_directory(p))
          push_back(p);
      }
  }
  push_back(".");
}

    
boost::filesystem::path sharedModelFilePath(const std::string& name)
{
    sharedModelLocations paths;

    for (const boost::filesystem::path& ps: paths)
    {
        path p(ps/name);
        if (exists(p))
        {
            return p;
        }
    }

    throw insight::Exception(_("Shared model file %s not found."), name.c_str());
    return boost::filesystem::path();
}




namespace parser {  


    
    
using namespace qi;
using namespace phx;
using namespace insight::cad;


ostream &operator<<(ostream &os, const SyntaxElementLocation &sel)
{
    // TRANSLATORS: character range of a syntax element, e.g. "12 until 20"
    os << str(format(_("%d until %d")) % sel.second.first % sel.second.second);
    if (!sel.first.empty())
        os << " " << str(format(_("in file \"%s\"")) % sel.first.string());
    return os;
}

void SyntaxElementDirectory::addEntry(
    SyntaxElementLocation location, FeaturePtr element)
{
    // remove, if already contained
    if (count(location)) erase(location);

    this->insert(
        std::pair<SyntaxElementLocation, SyntaxElement>(
            location, element ) );
}


void SyntaxElementDirectory::addFSEntry(
    SyntaxElementLocation location, FeatureSetPtr element)
{
    // remove, if already contained
    if (count(location)) erase(location);

    this->insert(
        std::pair<SyntaxElementLocation, SyntaxElement>(
            location, element ) );
}


SyntaxElement SyntaxElementDirectory::findElement(
    long location, const boost::filesystem::path& file ) const
{
    for (const value_type& elem: *this)
    {
      SyntaxElementLocation l=elem.first;
        if ( (l.second.first<=location) && (l.second.second>=location) )
            return elem.second;
    }
    return FeaturePtr();
}



skip_grammar::skip_grammar()
    : skip_grammar::base_type(skip, "PL/0")
{
    skip
        =   qi::char_(" \t\n\r\f\v") // not isspace since this causes segfault with non-ascii-characters
            | repo::confix("/*", "*/")[*(qi::char_ - "*/")]
            | ("//" >> *(qi::char_ - qi::eol))
            | ("#"  >> *(qi::char_ - qi::eol))
            ;
}



SubmodelRule::SubmodelRule(
    const Model* parentModel,
    const ModelVariableTable& addVars )
  : submodel_(std::make_shared<Model>(
          mergeMVTs(parentModel->allVariables(),
                    addVars) ) ),
    submodelParser_(submodel_.get())
{}

const qi::rule<std::string::iterator, skip_grammar>&
SubmodelRule::rule() const
{
    return submodelParser_.r_model;
}



/** \page iscad ISCAD
 *
 * \section intro ISCAD Parser Language
 * 
 * ISCAD is an interpreter for the insight modelling language for creation of CAD models.
 * 
 * The first section of an ISCAD model script consists of a number of \subpage iscad_assignments "assignments" which create symbols.
 * These symbols can represent scalar or vector parameters, datums, modelling geometry or sets of features (vertices, edges, faces or volumes).
 * 
 * After the modelling section, an optional postprocessing section can be started by the statement "\@ post". 
 * In this section, an arbitrary number of \subpage iscad_postprocessing_commands can be given.
 * 
 * \section iscad_commands Commands
 * * \subpage iscad_arc
 * * \subpage iscad_bar
 */





// template <typename Iterator, typename Skipper = skip_grammar<Iterator> >
ISCADParser::ISCADParser(Model* model, const boost::filesystem::path& filenameinfo)
    : insight::ExtendedGrammar<qi::grammar<std::string::iterator, skip_grammar> >(r_model),
      filenameinfo_(filenameinfo),
      syntax_element_locations(new SyntaxElementDirectory()),
      model_(model)
{

    r_descriptionWithParameters =
        ( r_string > (('%' > r_scalarExpression % '%')
                      | qi::attr(std::vector<ScalarPtr>())) )
        [ qi::_val = insight::cad::parser::make_shared_<DescriptionWithParameters>()(qi::_1, qi::_2) ] ;
    r_descriptionWithParameters.name(_("description"));


    r_BOMDescriptionData =
        ( r_descriptionWithParameters >
         ( ( '(' > r_descriptionWithParameters > ')' ) | qi::attr(DescriptionWithParametersPtr()) ) )
        [ qi::_val = insight::cad::parser::make_shared_<BOMDescriptionData>()(qi::_1, qi::_2) ]
        ;
    r_BOMDescriptionData.name(_("BOM description"));

    r_model =
        ( (kw("cost") >> iscad_double >> ';' ) | qi::attr(0.0) )
            [ phx::bind( &Model::setCost, model_, qi::_1 ) ]
        >>
        *(
          r_assignment
          |
          r_solidmodel_propertyAssignment
          |
          (qi::lit("@description")
                > r_BOMDescriptionData
                > ';' )
              [ phx::bind( &Model::setDescription, model_, qi::_1 ) ]

        )
        >> -( lit("@doc") [ phx::ref(section_) = DocSection ] > *r_doc )
        >> -( lit("@post") [ phx::ref(section_) = PostSection ] > *r_postproc )
        ;
    r_model.name(_("model description"));


    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
    r_identifier.name(_("identifier"));

    r_path = as_string[
                 lexeme [ "\"" > *~char_("\"") > "\"" ]
             ];
    r_path.name(_("path"));

    r_string = as_string[
                   lexeme [ "\'" > *~char_("\'") > "\'" ]
               ];
    r_string.name(_("string"));


    /*! \page iscad_assignments ISCAD Assignments
     *
     */
    r_assignment =
        // function names cannot be used as symbol names, since the
        // function would be picked up instead in expressions
        !lexeme[
            ( omit[modelstepFunctionRules] | omit[scalarFunctionRules]
            | omit[vectorFunctionRules] | omit[postProcFunctionRules] )
            >> !(alnum | '_') ]
        >>
        //                 1              2                         3                            4                     5
        ( current_pos.current_pos >> r_identifier >> current_pos.current_pos )
        [ qi::_a=qi::_2, qi::_b=phx::construct<SyntaxElementPos>(qi::_1, qi::_3) ]
        >> (
         ( ':' >
          //              1                2
          (r_solidmodel_expression > ( r_string | qi::attr(std::string()) ) > ';' )
             [ ( phx::bind(&Model::addModelstep, model_, qi::_a, qi::_1, true, qi::_2),
               phx::bind( &SyntaxElementDirectory::addEntry, syntax_element_locations.get(),
                         phx::construct<SyntaxElementLocation>(
                             filenameinfo_, qi::_b ),
                         qi::_1
                         )
               ) ]
         )
         |
         ( qi::lit("=") >
          (
              ( r_datumExpression >> ';' )
                [ phx::bind(&Model::addDatum, model_, qi::_a, qi::_1) ]

            | ( r_scalarExpression >> ';' )
                [ phx::bind(&Model::addScalar, model_, qi::_a, qi::_1) ]

            | ( r_vectorExpression >> ';' )
                [ phx::bind(&Model::addPoint, model_, qi::_a, qi::_1) ]

            | ( r_vertexFeaturesExpression >> ';' )
                [ phx::bind(&Model::addVertexFeature, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_edgeFeaturesExpression >> ';' )
                [ phx::bind(&Model::addEdgeFeature, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_faceFeaturesExpression >> ';' )
                [ phx::bind(&Model::addFaceFeature, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_solidFeaturesExpression >> ';' )
                [ phx::bind(&Model::addSolidFeature, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            //              1                2
            | (r_solidmodel_expression >> ( r_string | qi::attr(std::string()) ) >> ';' )
                [ ( phx::bind(&Model::addModelstep, model_, qi::_a, qi::_1, false, qi::_2),
                  phx::bind( &SyntaxElementDirectory::addEntry, syntax_element_locations.get(),
                           phx::construct<SyntaxElementLocation>(
                               filenameinfo_, qi::_b ),
                           qi::_1
                           )
                 ) ]
          )
         )
         |
         ( qi::lit("?=") >
           (
              ( r_datumExpression >> ';' )
               [ phx::bind(&Model::addDatumIfNotPresent, model_, qi::_a, qi::_1) ]

            | ( r_scalarExpression >> ';' )
               [ phx::bind(&Model::addScalarIfNotPresent, model_, qi::_a, qi::_1) ]
            | ( r_vectorExpression >> ';' )
               [ phx::bind(&Model::addPointIfNotPresent, model_, qi::_a, qi::_1) ]

            | ( r_vertexFeaturesExpression >> ';' )
                [ phx::bind(&Model::addVertexFeatureIfNotPresent, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_edgeFeaturesExpression >> ';' )
                [ phx::bind(&Model::addEdgeFeatureIfNotPresent, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_faceFeaturesExpression >> ';' )
                [ phx::bind(&Model::addFaceFeatureIfNotPresent, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_solidFeaturesExpression >> ';' )
                [ phx::bind(&Model::addSolidFeatureIfNotPresent, model_, qi::_a, qi::_1),
                    phx::bind( &SyntaxElementDirectory::addFSEntry, syntax_element_locations.get(),
                              phx::construct<SyntaxElementLocation>(
                                  filenameinfo_, qi::_b ),
                              qi::_1
                              ) ]

            | ( r_solidmodel_expression >> ( r_string | qi::attr(std::string()) ) >> ';' )
               [ ( phx::bind(&Model::addModelstepIfNotPresent, model_, qi::_a, qi::_1, false, qi::_2),
                  phx::bind( &SyntaxElementDirectory::addEntry, syntax_element_locations.get(),
                            phx::construct<SyntaxElementLocation>(
                                filenameinfo_, qi::_b ),
                            qi::_1
                            )
                  )]

           )
         )
        );

        // |
        // ( r_identifier >> lit("!=")  >> r_vectorExpression >> ';')
        // [ phx::bind(&Model::addDirection, model_, qi::_1, qi::_2) ]
        // |
        // ( r_identifier >> lit("?!=")  >> r_vectorExpression >> ';')
        // [ phx::bind(&Model::addDirectionIfNotPresent, model_, qi::_1, qi::_2) ]

    r_assignment.name(_("assignment"));

    createDocExpressions();
    createSelectionExpressions();
    createDatumExpressions();
    createScalarExpressions();
    createVectorExpressions();
    createFeatureExpressions();
    createPostProcExpressions();

    // name the argument rules of all commands (for error messages)
    // and remember the command names
    auto registerCommands = [this](const auto& table, CommandKind kind)
    {
        table.for_each(
            [this, kind](const std::string& name, const auto& rule)
            {
                rule->name(_("argument list"));
                commandKinds_[name]=kind;
            });
    };
    registerCommands(modelstepFunctionRules, FeatureCommand);
    registerCommands(scalarFunctionRules, ScalarFunction);
    registerCommands(vectorFunctionRules, VectorFunction);
    registerCommands(postProcFunctionRules, PostprocCommand);

    // BOOST_SPIRIT_DEBUG_RULE(r_assignment);
}




std::string ISCADParser::commandKindDescription(CommandKind kind)
{
    switch (kind)
    {
        case FeatureCommand: return _("feature command");
        case ScalarFunction: return _("scalar function");
        case VectorFunction: return _("vector function");
        case PostprocCommand: return _("postprocessing command");
    }
    return std::string();
}




void ISCADParser::pushCommand(std::size_t nameBegin, std::size_t nameEnd)
{
    commandStack_.push_back(SyntaxElementPos(nameBegin, nameEnd));
}




void ISCADParser::popCommand()
{
    if (!commandStack_.empty())
        commandStack_.pop_back();
}






}

using namespace parser;


bool parseISCADModelFile(
    const boost::filesystem::path& fn,
    Model* m,
    int* failloc,
    parser::SyntaxElementDirectoryPtr* sd )
{
    if (!boost::filesystem::exists(fn))
    {
        throw insight::Exception(_("The iscad script file \"%s\" does not exist!"), fn.string().c_str());
        return false;
    }
    
    std::ifstream f(fn.string());
    insight::assertion(
                f.good(),
                _("could not read iscad script file \"%s\""), fn.string().c_str() );
    return parseISCADModelStream(f, m, failloc, sd, fn);
}


iscadParserException::iscadParserException(const std::string& reason, int from_pos, int to_pos)
: Exception("%s", reason.c_str()),
  from_pos_(from_pos),
  to_pos_(to_pos),
  diagnostic_(reason)
{
}




iscadParserException::iscadParserException(
    const std::string& diagnostic,
    const std::vector<std::string>& notes,
    const std::string& script,
    const boost::filesystem::path& file,
    int from_pos, int to_pos )
: Exception("%s", formatDiagnostic(file, script, from_pos, to_pos, diagnostic, notes).c_str()),
  from_pos_(from_pos),
  to_pos_(to_pos),
  file_(file),
  diagnostic_(diagnostic)
{
    std::copy_if(
        notes.begin(), notes.end(), std::back_inserter(notes_),
        [](const std::string& n) { return !n.empty(); } );

    if (from_pos>=0 && std::size_t(from_pos)<=script.size())
    {
        auto loc=locateInSource(script, from_pos);
        line_=loc.line;
        column_=loc.column;
    }
}




std::string iscadParserException::summary() const
{
    std::ostringstream os;
    if (line_>0)
        os << str(format(_("line %d, column %d")) % line_ % column_) << ": ";
    os << diagnostic_;
    for (const auto& n: notes_)
        os << " " << n;
    return os.str();
}




namespace
{


std::string commandName(const std::string& script, const SyntaxElementPos& p)
{
    return script.substr(p.first, p.second-p.first);
}


std::string featureCommandUsage(const std::string& cmd)
{
    if (Feature::ruleDocumentationFunctions_)
    {
        for (const auto& rd: *Feature::ruleDocumentationFunctions_)
        {
            for (const auto& info: rd.second())
            {
                if (info.command_==cmd)
                {
                    std::string sig=info.signature_;
                    boost::replace_all(sig, "\n", " ");
                    boost::trim(sig);
                    return str(format(_("Usage: %s")) % (info.command_+sig));
                }
            }
        }
    }
    return std::string();
}


std::set<std::string> knownNames(const ISCADParser& parser)
{
    auto names = parser.model_->symbolNames();
    for (const auto& c: parser.commandKinds_)
    {
        // postprocessing commands are only valid in the @post section
        if ( (c.second==ISCADParser::PostprocCommand)
             == (parser.section_==ISCADParser::PostSection) )
            names.insert(c.first);
    }
    return names;
}


bool isLiteral(const std::string& expectedItem)
{
    return expectedItem.size()>=2 && expectedItem.front()=='\'';
}


/**
 * error location and description of a missing terminator:
 * report it at the end of the preceding token, if that is on a previous line
 */
std::size_t endOfPrecedingToken(const std::string& script, std::size_t pos)
{
    std::size_t p=pos;
    while (p>0 && std::isspace(static_cast<unsigned char>(script[p-1])))
        --p;
    if (script.substr(p, pos-p).find('\n')!=std::string::npos)
        return p;
    return pos;
}


iscadParserException expectationError(
    const ISCADParser& parser,
    const std::string& script,
    const boost::filesystem::path& file,
    std::size_t failPos,
    const boost::spirit::info& what )
{
    auto expected = expectedAlternatives(what);
    std::size_t pos = skipWhitespaceAndComments(script, failPos);
    std::size_t reportPos = pos;

    std::vector<std::string> notes;
    std::string diag;

    bool onlyLiterals = std::all_of(expected.begin(), expected.end(), isLiteral);

    auto id = identifierAt(script, pos);
    std::vector<std::string> kinds;
    if (!id.empty() && !parser.commandKinds_.count(id))
        kinds = parser.model_->symbolKinds(id);

    if (!id.empty() && !parser.commandKinds_.count(id)
        && kinds.empty() && !onlyLiterals)
    {
        diag = str(format(_("undefined symbol '%s'")) % id);
        notes.push_back(didYouMean(similarNames(id, knownNames(parser))));
        notes.push_back(str(format(_("Expected here: %s.")) % describeAlternatives(expected)));
    }
    else
    {
        std::string found = describeToken(script, pos);
        if (!id.empty())
        {
            auto ck = parser.commandKinds_.find(id);
            if (ck!=parser.commandKinds_.end())
                found += " ("+ISCADParser::commandKindDescription(ck->second)+")";
            else if (!kinds.empty())
                found += " ("+str(format(_("defined as %s")) % boost::join(kinds, ", "))+")";
            else
            {
                // maybe a misspelled keyword
                std::set<std::string> keywords;
                for (const auto& e: expected)
                    if (isLiteral(e))
                        keywords.insert(e.substr(1, e.size()-2));
                notes.push_back(didYouMean(similarNames(id, keywords)));
            }
        }

        // TRANSLATORS: first %s: list of alternatives, second %s: the token found in the script
        diag = str(format(_("expected %s but found %s")) % describeAlternatives(expected) % found);

        if (onlyLiterals)
            reportPos = endOfPrecedingToken(script, pos);
    }

    if (!parser.commandStack_.empty())
    {
        auto cmd = commandName(script, parser.commandStack_.back());
        // TRANSLATORS: first %s: command name, second %s: error message
        diag = str(format(_("in %s(...): %s")) % cmd % diag);
        notes.push_back(featureCommandUsage(cmd));
    }

    notes.push_back(missingSemicolonHint(script, pos));

    std::size_t len = std::max<std::size_t>(1, tokenLength(script, reportPos));
    return iscadParserException(
        diag, notes, script, file,
        int(reportPos), int(std::min(reportPos+len, script.size())) );
}


std::string statementForms()
{
    return _("Statements have the form 'name = expression;', 'name: feature expression;'"
             " or 'feature -> property = value;'.");
}


iscadParserException incompleteParseError(
    const ISCADParser& parser,
    const std::string& script,
    const boost::filesystem::path& file,
    std::size_t failPos )
{
    std::size_t pos = skipWhitespaceAndComments(script, failPos);
    std::vector<std::string> notes;
    std::string diag;

    auto id = identifierAt(script, pos);

    if (pos>=script.size())
    {
        diag = _("unexpected end of input");
    }
    else if (script[pos]=='@')
    {
        std::string word = "@"+identifierAt(script, pos+1);
        std::set<std::string> sections = { "@description", "@doc", "@post" };
        if (!sections.count(word))
        {
            diag = str(format(_("unknown keyword '%s'")) % word);
            notes.push_back(didYouMean(similarNames(word, sections)));
            notes.push_back(_("Valid keywords are '@description', '@doc' and '@post'."));
        }
        else
        {
            diag = str(format(_("'%s' is not allowed here")) % word);
            notes.push_back(
                _("A script consists of assignments (and '@description'),"
                  " optionally followed by an '@doc' section and then an '@post' section.") );
        }
    }
    else if (!id.empty())
    {
        std::size_t next = skipWhitespaceAndComments(script, pos+id.size());
        auto followedBy = [&](const char* s)
        {
            return script.compare(next, std::strlen(s), s)==0;
        };
        bool assignment = followedBy(":") || followedBy("=") || followedBy("?=");
        auto ck = parser.commandKinds_.find(id);
        auto kinds = parser.model_->symbolKinds(id);

        if (parser.section_==ISCADParser::PostSection)
        {
            diag = str(format(_("unknown postprocessing command '%s'")) % id);
            std::set<std::string> ppcmds;
            for (const auto& c: parser.commandKinds_)
                if (c.second==ISCADParser::PostprocCommand)
                    ppcmds.insert(c.first);
            notes.push_back(didYouMean(similarNames(id, ppcmds)));
        }
        else if (parser.section_==ISCADParser::DocSection)
        {
            diag = str(format(_("cannot parse documentation statement starting with '%s'")) % id);
        }
        else if (ck!=parser.commandKinds_.end() && assignment)
        {
            // TRANSLATORS: first %s: symbol name, second %s: kind of command, e.g. "feature command"
            diag = str(format(_("'%s' cannot be used as a symbol name (%s of the same name exists)"))
                       % id % ISCADParser::commandKindDescription(ck->second) );
            notes.push_back(_("Please choose a different name."));
        }
        else if (followedBy("->"))
        {
            if (kinds.empty())
            {
                diag = str(format(_("undefined feature '%s'")) % id);
                notes.push_back(didYouMean(similarNames(id, parser.model_->symbolNames())));
            }
            else
            {
                // TRANSLATORS: first %s: symbol name, second %s: list of symbol kinds, e.g. "scalar"
                diag = str(format(_("properties can only be assigned to features, but '%s' is defined as %s"))
                           % id % boost::join(kinds, ", "));
            }
        }
        else if (assignment)
        {
            diag = str(format(_("cannot parse assignment to '%s'")) % id);
        }
        else
        {
            // TRANSLATORS: first %s: symbol name, second %s: the token found in the script
            diag = str(format(_("expected ':', '=', '?=' or '->' after '%s' but found %s"))
                       % id % describeToken(script, next));
            notes.push_back(statementForms());
        }
    }
    else
    {
        diag = str(format(_("unexpected %s at the beginning of a statement"))
                   % describeToken(script, pos));
        notes.push_back(statementForms());
    }

    std::size_t len = std::max<std::size_t>(1, tokenLength(script, pos));
    return iscadParserException(
        diag, notes, script, file,
        int(pos), int(std::min(pos+len, script.size())) );
}


/**
 * error which was raised inside a semantic action:
 * locate it at the innermost command
 */
iscadParserException semanticError(
    const ISCADParser& parser,
    const std::string& script,
    const boost::filesystem::path& file,
    const std::string& message )
{
    int from=-1, to=-1;
    std::string diag=message;
    if (!parser.commandStack_.empty())
    {
        const auto& c = parser.commandStack_.back();
        from=int(c.first);
        to=int(c.second);
        diag = str(format(_("in %s(...): %s")) % commandName(script, c) % message);
    }
    return iscadParserException(diag, {}, script, file, from, to);
}


[[noreturn]] void throwParserError(const iscadParserException& e, int* failloc)
{
    if (failloc) *failloc=e.from_pos();
    throw e;
}


} // anonymous namespace




bool parseISCADModel
(
    const std::string& script,
    Model* m,
    int* failloc,
    parser::SyntaxElementDirectoryPtr* sd,
    const boost::filesystem::path& filenameinfo
)
{
    std::string raw_contents(script);

    std::string::iterator orgbegin,
        first=raw_contents.begin(),
        last=raw_contents.end();

    orgbegin=first;

    ISCADParser parser ( m, filenameinfo );
    if ( sd ) *sd = parser.syntax_element_locations;

    bool r = false;
    try
    {
        skip_grammar skip;

        parser.current_pos.setStartPos ( first );
        r = qi::phrase_parse (
            first,
            last,
            parser,
            skip
            );
    }
    catch ( const qi::expectation_failure<std::string::iterator>& e )
    {
        throwParserError(
            expectationError(parser, raw_contents, filenameinfo, e.first-orgbegin, e.what_),
            failloc );
    }
    catch ( const iscadParserException& e )
    {
        if (e.file()==filenameinfo)
        {
            if (failloc) *failloc=e.from_pos();
            throw;
        }
        // error in another (included) script
        throwParserError( semanticError(parser, raw_contents, filenameinfo, e.message()), failloc );
    }
    catch ( const insight::Exception& e )
    {
        throwParserError( semanticError(parser, raw_contents, filenameinfo, e.message()), failloc );
    }

    if ( !r || first != last ) // fail if we did not get a full match
    {
        throwParserError(
            incompleteParseError(parser, raw_contents, filenameinfo, first-orgbegin),
            failloc );
    }

    return true;
}



bool parseISCADModelStream (
    std::istream& in,
    Model* m,
    int* failloc,
    parser::SyntaxElementDirectoryPtr* sd,
    const boost::filesystem::path& filenameinfo
)
{
  in >> std::noskipws;

// use stream iterators to copy the stream to a string
  std::istream_iterator<char> it(in);
  std::istream_iterator<char> end;
  std::string contents_raw(it, end);

  return parseISCADModel(contents_raw, m, failloc, sd, filenameinfo);
}




}
}
