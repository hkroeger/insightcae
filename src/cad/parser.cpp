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


    
    
boost::filesystem::path sharedModelFilePath(const std::string& name)
{
    std::vector<boost::filesystem::path> paths;
    const char* e=getenv("ISCAD_MODEL_PATH");
    if (e)
    {
        boost::split(paths, e, boost::is_any_of(":"));
    }
    {
//         insight::SharedPathList spl;
        BOOST_FOREACH(const path& p, insight::SharedPathList::searchPathList)
	{
	  if (boost::filesystem::is_directory(p/"iscad-library"))
	    paths.push_back(p/"iscad-library");
	  else if (boost::filesystem::is_directory(p))
	    paths.push_back(p);
	}
    }
    paths.push_back(".");

    BOOST_FOREACH(const boost::filesystem::path& ps, paths)
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




void SyntaxElementDirectory::addEntry(SyntaxElementLocation location, FeaturePtr element)
{
//     std::cout<<"between START="<<location.first<<" END="<<location.second<<std::endl;
    this->insert(std::pair<SyntaxElementLocation, FeaturePtr>(location, element));
}




FeaturePtr SyntaxElementDirectory::findElement(size_t location) const
{
    BOOST_FOREACH(const value_type& elem, *this)
    {
        if ( (elem.first.first<=location) && (elem.first.second>=location) )
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


AddRuleContainerBase::~AddRuleContainerBase()
{}



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
ISCADParser::ISCADParser(Model* model)
    : ISCADParser::base_type(r_model),
      syntax_element_locations(new SyntaxElementDirectory()),
      model_(model)
{
    r_model =
//      current_pos.save_start_pos >>
        *(
            r_assignment
            |
            r_modelstep
            |
            r_solidmodel_propertyAssignment
        )
        >> -( lit("@doc") > *r_doc )
        >> -( lit("@post") > *r_postproc )
        ;
    r_model.name("model description");


    r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
    r_identifier.name("identifier");

    r_path = as_string[
                 lexeme [ "\"" >> *~char_("\"") >> "\"" ]
             ];
    r_path.name("path");

    r_string = as_string[
                   lexeme [ "\'" >> *~char_("\'") >> "\'" ]
               ];
    r_string.name("string");


    /*! \page iscad_assignments ISCAD Assignments
     *
     */
    r_assignment =
        ( r_identifier >> '=' >> r_solidmodel_expression >> ( r_string | qi::attr(std::string()) ) >> ';' )
        [ phx::bind(&Model::addModelstep, model_, qi::_1, qi::_2, qi::_3) ]
        |
        ( r_identifier >> lit("?=") >> r_solidmodel_expression >> ( r_string | qi::attr(std::string()) ) >> ';' )
        [ phx::bind(&Model::addModelstepIfNotPresent, model_, qi::_1, qi::_2, qi::_3) ]
        |
        ( r_identifier >> '='  >> r_datumExpression >> ';')
        [ phx::bind(&Model::addDatum, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> lit("?=")  >> r_datumExpression >> ';')
        [ phx::bind(&Model::addDatumIfNotPresent, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> '='  >> r_scalarExpression >> ';')
        [ phx::bind(&Model::addScalar, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> lit("?=")  >> r_scalarExpression >> ';')
        [ phx::bind(&Model::addScalarIfNotPresent, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> '='  >> r_vectorExpression >> ';')
        [ phx::bind(&Model::addVector, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> lit("?=")  >> r_vectorExpression >> ';')
        [ phx::bind(&Model::addVectorIfNotPresent, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> '='  >> r_vertexFeaturesExpression >> ';')
        [ phx::bind(&Model::addVertexFeature, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> '='  >> r_edgeFeaturesExpression >> ';')
        [ phx::bind(&Model::addEdgeFeature, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> '='  >> r_faceFeaturesExpression >> ';')
        [ phx::bind(&Model::addFaceFeature, model_, qi::_1, qi::_2) ]
        |
        ( r_identifier >> '='  >> r_solidFeaturesExpression >> ';')
        [ phx::bind(&Model::addSolidFeature, model_, qi::_1, qi::_2) ]
        ;
    r_assignment.name("assignment");


    r_modelstep  =  ( r_identifier >> ':' >> r_solidmodel_expression >> ( r_string | qi::attr(std::string()) ) >> ';' )
                    [ phx::bind(&Model::addComponent, model_, qi::_1, qi::_2, qi::_3) ]
//       [ (phx::bind(&Model::addComponent, model_, qi::_2, qi::_3), std::cout<<"POS="<<qi::_1<<std::endl ) ]
                    ;
    r_modelstep.name("modelling step");


    createPostProcExpressions();
    createDocExpressions();
    createFeatureExpressions();
    createSelectionExpressions();
    createDatumExpressions();
    createScalarExpressions();
    createVectorExpressions();


#if (INSIGHT_CAD_DEBUG>2)
    BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_expression);
    BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);
    BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
    BOOST_SPIRIT_DEBUG_NODE(r_modelstep);
    BOOST_SPIRIT_DEBUG_NODE(r_modelstepFunction);
    BOOST_SPIRIT_DEBUG_NODE(r_path);
    BOOST_SPIRIT_DEBUG_NODE(r_identifier);
    BOOST_SPIRIT_DEBUG_NODE(r_assignment);
    BOOST_SPIRIT_DEBUG_NODE(r_postproc);
    BOOST_SPIRIT_DEBUG_NODE(r_viewDef);
    BOOST_SPIRIT_DEBUG_NODE(r_scalarExpression);
    BOOST_SPIRIT_DEBUG_NODE(r_vector_primary);
    BOOST_SPIRIT_DEBUG_NODE(r_vector_term);
    BOOST_SPIRIT_DEBUG_NODE(r_vectorExpression);
    BOOST_SPIRIT_DEBUG_NODE(r_model);
    BOOST_SPIRIT_DEBUG_NODE(r_vertexFeaturesExpression);
// 	BOOST_SPIRIT_DEBUG_NODE(r_solidmodel_propertyAssignment);
    BOOST_SPIRIT_DEBUG_NODE(r_edgeFeaturesExpression);
#endif

    on_error<fail>(r_model,
                   phx::ref(std::cout)
                   << "Error! Expecting "
                   << qi::_4
                   << " here: '"
                   << phx::construct<std::string>(qi::_3, qi::_2)
                   << "'\n"
                  );
}




bool parseISCADModel(std::string::iterator first, std::string::iterator last, Model* model)
{
  ISCADParser parser(model);
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


bool parseISCADModelFile(const boost::filesystem::path& fn, Model* m, int* failloc, parser::SyntaxElementDirectoryPtr* sd)
{
    if (!boost::filesystem::exists(fn))
    {
        throw insight::Exception("The iscad script file \""+fn.string()+"\" does not exist!");
        return false;
    }
    
    std::ifstream f(fn.c_str());
    return parseISCADModelStream(f, m, failloc, sd);
}

bool parseISCADModelStream(std::istream& in, Model* m, int* failloc, parser::SyntaxElementDirectoryPtr* sd)
{
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  
  std::string::iterator orgbegin,
    first=contents_raw.begin(), 
    last=contents_raw.end();
    
  orgbegin=first;
  
  ISCADParser parser(m);
  skip_grammar skip;
  
//   std::cout<<"Parsing started."<<std::endl;
  parser.current_pos.setStartPos(first);
  bool r = qi::phrase_parse(
      first,
      last,
      parser,
      skip
  );
//   std::cout<<"Parsing finished."<<std::endl;
  
  if (first != last) // fail if we did not get a full match
  {
    if (failloc) *failloc=int(first-orgbegin);
    return false;
  }
  else
  {
      if (sd) 
      {
          *sd = parser.syntax_element_locations;
      }
  }
  
  return r;
}

}
}
