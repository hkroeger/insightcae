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
#ifdef INSIGHT_CAD_DEBUG
#define BOOST_SPIRIT_DEBUG
#endif


#include "cadfeature.h"

#include "datum.h"
#include "sketch.h"
#include "cadpostprocactions.h"

#include "base/analysis.h"
#include "parser.h"
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
  CurrentExceptionContext ec("building list of shared model locations");

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

    throw insight::Exception("Shared model file "+name+" not found.");
    return boost::filesystem::path();
}




namespace parser {  


    
    
using namespace qi;
using namespace phx;
using namespace insight::cad;


ostream &operator<<(ostream &os, const SyntaxElementLocation &sel)
{
    os << sel.second.first << " until " << sel.second.second;
    if (!sel.first.empty())
        os << " in file \""<<sel.first<<"\"";
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
        =   boost::spirit::ascii::space
            | repo::confix("/*", "*/")[*(qi::char_ - "*/")]
            | repo::confix("//", qi::eol)[*(qi::char_ - qi::eol)]
            | repo::confix("#", qi::eol)[*(qi::char_ - qi::eol)]
            ;
}



SubmodelRule::SubmodelRule(const cad::Model& parentModel, const ModelVariableTable& addVars)
: submodel_(std::make_shared<Model>(
             mergeMVTs(parentModel.allVariables(), addVars) ) ),
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
        [ qi::_val = phx::make_shared_<DescriptionWithParameters>()(qi::_1, qi::_2) ] ;


    r_BOMDescriptionData =
        ( r_descriptionWithParameters >
         ( ( '(' > r_descriptionWithParameters > ')' ) | qi::attr(DescriptionWithParametersPtr()) ) )
        [ qi::_val = phx::make_shared_<BOMDescriptionData>()(qi::_1, qi::_2) ]
        ;

    r_model =
        ( (qi::lit("cost") >> qi::double_ >> ';' ) | qi::attr(0.0) )
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
        >> -( lit("@doc") > *r_doc )
        >> -( lit("@post") > *r_postproc )
        ;
    r_model.name("model description");


    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
    r_identifier.name("identifier");

    r_path = as_string[
                 lexeme [ "\"" > *~char_("\"") > "\"" ]
             ];
    r_path.name("path");

    r_string = as_string[
                   lexeme [ "\'" > *~char_("\'") > "\'" ]
               ];
    r_string.name("string");


    /*! \page iscad_assignments ISCAD Assignments
     *
     */
    r_assignment =
        //                 1              2                         3                            4                     5
        ( current_pos.current_pos >> r_identifier >> current_pos.current_pos )
        [ qi::_a=qi::_2, qi::_b=phx::construct<SyntaxElementPos>(qi::_1, qi::_3) ]
        >> (
         ( ':' >
          //              1                2
          (r_solidmodel_expression >> ( r_string | qi::attr(std::string()) ) >> ';' )
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

    r_assignment.name("assignment");

    // on_error<qi::fail>
    //     (
    //         r_assignment, std::cout
    //             << val("Error! Expecting ")
    //             << qi::_4                               // what failed?
    //             << val(" here: \"")
    //             << construct<std::string>(qi::_3, qi::_2)   // iterators to error-pos, end
    //             << val("\"")
    //             << std::endl
    //         );

    createDocExpressions();
    createSelectionExpressions();
    createDatumExpressions();
    createScalarExpressions();
    createVectorExpressions();
    createFeatureExpressions();
    createPostProcExpressions();

    // BOOST_SPIRIT_DEBUG_RULE(r_assignment);
}



bool parseISCADModel(
    std::string::iterator first,
    std::string::iterator last,
    Model* model,
    const boost::filesystem::path& filenameinfo )
{
  ISCADParser parser(model, filenameinfo);
  skip_grammar skip;
  
  std::cout<<"Parsing started."<<std::endl;
  parser.current_pos.setStartPos(first);
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
  std::cout<<"Parsing finished."<<std::endl;
  
  if (first != last) return false;
  return r;
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
        throw insight::Exception("The iscad script file \""+fn.string()+"\" does not exist!");
        return false;
    }
    
    std::ifstream f(fn.string());
    insight::assertion(
                f.good(),
                "stream not good!");
    return parseISCADModelStream(f, m, failloc, sd, fn);
}


iscadParserException::iscadParserException(const std::string& reason, int from_pos, int to_pos)
: Exception(reason, false),
  from_pos_(from_pos),
  to_pos_(to_pos)
{
}


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

    bool r = false;
    try
    {

        ISCADParser parser ( m, filenameinfo );
        if ( sd ) *sd = parser.syntax_element_locations;

        skip_grammar skip;

        //   std::cout<<"Parsing started."<<std::endl;
        parser.current_pos.setStartPos ( first );
        r = qi::phrase_parse (
            first,
            last,
            parser,
            skip
            );
        //   std::cout<<"Parsing finished."<<std::endl;

        if ( first != last ) // fail if we did not get a full match
        {
            if ( failloc ) *failloc=int ( first-orgbegin );
            return false;
        }
    }
    catch ( const qi::expectation_failure<std::string::iterator>& e )
    {
        std::ostringstream os;
        os << e.what_;
        throw iscadParserException(os.str(), int(e.first-orgbegin), int(e.last-orgbegin));
    }
    return r;
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
