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


#include "boolparameterparser.h"
#include "doubleparameterparser.h"
#include "vectorparameterparser.h"
#include "stringparameterparser.h"
#include "pathparameterparser.h"
#include "intparameterparser.h"
#include "subsetparameterparser.h"
#include "includedsubsetparameterparser.h"
#include "selectionparameterparser.h"
#include "arrayparameterparser.h"
#include "doublerangeparameterparser.h"
#include "dynamicclassselectablesubsetparameterparser.h"
#include "dynamicclassparametersselectablesubsetparameterparser.h"
#include "matrixparameterparser.h"
#include "selectablesubsetparameterparser.h"




/**
 * \defgroup PDL PDL - Parameter Definition Language
 *
 * PDL is a special syntax to describe input parameter sets.
 * The language is parsed by a PDL compiler who translates it into special classes which..
 */







using namespace std;
using namespace qi;
using namespace phx;
using namespace boost;



template <typename Iterator>
skip_grammar<Iterator>::skip_grammar() 
  : skip_grammar::base_type(skip, "PL/0")
{
  skip
      =   boost::spirit::ascii::space
          | repo::confix("//", qi::eol)[*(qi::char_ - qi::eol)]
          | repo::confix("#", qi::eol)[*(qi::char_ - qi::eol)]
          ;
}
/**
 * \addtogroup PDL
 * \section grammar Grammar
 * \subsection grammar_comments Comments
 * Comments are supported inside the definition block:
 * - lines starting with //
 * - lines starting with #
 */


template <typename Iterator, typename Skipper>
PDLParserRuleset<Iterator,Skipper>::PDLParserRuleset()
{  
  r_string = as_string[ lexeme [ "\"" >> *~char_("\"") >> "\"" ] ];
  r_description_string = (r_string | attr(""));
  r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
  r_path = lexeme[ alpha >> *(alnum | char_('_') | char_('/') ) >> !(alnum | '_' | '/' ) ];
  r_up_to_semicolon = qi::as_string[ qi::lexeme [ *~(qi::char_(";")) >> ";" ] ];

  r_parameterdata %= omit[ parameterDataRules[ qi::_a = qi::_1 ] ] > qi::lazy(*qi::_a);
  
  //   parameterDataRules.add
  //   (
  //     "include",
  //     typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRulePtr(new typename PDLParserRuleset<Iterator,Skipper>::ParameterDataRule(
  //       ( "(" >> r_string >> ")" >> ruleset.r_description_string )
  //       [ qi::_val = phx::construct<ParserDataBase::Ptr>(new_<Data>(phx::construct<arma::mat>(qi::_1), qi::_2)) ]
  //     ))
  //   );

  r_parametersetentry = r_identifier >> '=' > r_parameterdata;
  r_parameterset = *( r_parametersetentry );
  
  //   BOOST_SPIRIT_DEBUG_NODE(r_identifier);
  //   BOOST_SPIRIT_DEBUG_NODE(r_parameterdata);
  //   BOOST_SPIRIT_DEBUG_NODE(r_parameterset);
  //   BOOST_SPIRIT_DEBUG_NODE(r_parametersetentry);
}
/**
 * \addtogroup PDL
 * \subsection grammar_primitives Primitives
 * - Strings are any char confined inside quotes ""
 * - Descriptor strings may be omitted. In this case, an empty descriptor is inserted.
 * - Identifiers must not be enclosed by quotes but have to start with an alphabetical character.
 *   They may then contain alphanumerical chars or underscores.
 */




template <typename Iterator, typename Skipper>
struct PDLParser
    : qi::grammar< Iterator, ParameterSetData(), Skipper >
{

  public:
  
  PDLParserRuleset<Iterator,Skipper> rules;

  PDLParser()
  : PDLParser::base_type(rules.r_parameterset)
  {
    BoolParameterParser::insertrule<Iterator, Skipper>(rules);
    DoubleParameterParser::insertrule<Iterator, Skipper>(rules);
    VectorParameterParser::insertrule<Iterator, Skipper>(rules);
    StringParameterParser::insertrule<Iterator, Skipper>(rules);
    PathParameterParser::insertrule<Iterator, Skipper>(rules);
    IntParameterParser::insertrule<Iterator, Skipper>(rules);
    SubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    IncludedSubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    SelectionParameterParser::insertrule<Iterator, Skipper>(rules);
    ArrayParameterParser::insertrule<Iterator, Skipper>(rules);
    DoubleRangeParameterParser::insertrule<Iterator, Skipper>(rules);
    SelectableSubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    DynamicClassSelectableSubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    DynamicClassParametersSelectableSubsetParameterParser::insertrule<Iterator, Skipper>(rules);
    MatrixParameterParser::insertrule<Iterator, Skipper>(rules);
    
    rules.init();

    on_error<fail>(rules.r_parameterset,
                   phx::ref(std::cout)
                   << "Error! Expecting "
                   << qi::_4
                   << " here: '"
                   << phx::construct<std::string>(qi::_3, qi::_2)
                   << "'\n"
                   );
  }

};



int main ( int argc, char *argv[] )
{
  for ( int k=1; k<argc; k++ )
  {
    boost::filesystem::path inf ( argv[k] );
    try
    {
      cout<<"Processing PDL "<<inf<<endl;

      std::ifstream in ( inf.c_str() );

      if ( !in.good() )
      {
        exit ( -1 );
      }

      PDLParser<std::string::iterator> parser;
      skip_grammar<std::string::iterator> skip;

      std::string first_word, base_type_name="";
      in>>first_word;
      if ( first_word=="inherits" )
      {
        in>>base_type_name;
      }
      else
      {
        in.seekg ( 0, std::ios::beg );
      }

      std::istreambuf_iterator<char> eos;
      std::string contents_raw ( std::istreambuf_iterator<char> ( in ), eos );

      std::string::iterator first=contents_raw.begin();
      std::string::iterator last=contents_raw.end();

      ParameterSetData result;
      if (!qi::phrase_parse
          (
            first,
            last,
            parser,
            skip,
            result
            ))
      {
        throw PDLException("Parsing PDL "+inf.string()+" failed!");
      }

      {
        std::string bname=inf.stem().string();
        std::vector<std::string> parts;
        boost::algorithm::split_regex ( parts, bname, boost::regex ( "__" ) );
        std::string name=bname;
        if ( parts.size() ==3 )
        {
          name=parts[2];
        }


        {
          std::ofstream f ( bname+"_headers.h" );
          std::set<std::string> headers;
          for ( const ParameterSetEntry& pe: result )
          {
            pe.second->cppAddHeader ( headers );
          }
          for ( const std::string& h: headers )
          {
            f<<"#include "<<h<<endl;
          }
        }
        {
          std::ofstream f ( bname+".h" );

          f<<"struct "<<name<<endl;
          if ( !base_type_name.empty() )
          {
            f<<": public "<<base_type_name<<endl;
          }
          f<<"{"<<endl;

          // declare variables and types
          for ( const ParameterSetEntry& pe: result )
          {
            pe.second->writeCppHeader ( f, pe.first );
          }

          f<<name<<"()"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" : "<<base_type_name<<"()"<<endl;
          }
          f<<"{"<<endl;
          f<<" get(makeDefault());"<<endl;
          f<<"}"<<endl;

          //get from other ParameterSet
          f<<name<<"(const insight::ParameterSet& p)"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" : "<<base_type_name<<"(p)"<<endl;
          }
          f <<"{"<<endl
           <<" get(p);"<<endl
          <<"}"<<endl
            ;

          f<<"virtual ~"<<name<<"()"<<endl;
          f<<"{}"<<endl;

          //set into other ParameterSet
          f<<"void set(insight::ParameterSet& p) const"<<endl
          <<"{"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" "<<base_type_name<<"::set(p);"<<endl;
          }
          for ( const ParameterSetEntry& pe: result )
          {
            std::string subname=pe.first;
            f<<"{"<<endl;
            f<<pe.second->cppParamType ( subname ) <<"& "<<subname<<" = p.get< "<<pe.second->cppParamType ( subname ) <<" >(\""<<subname<<"\");"<<endl;
            f<<"const "<<pe.second->cppTypeName ( subname ) <<"& "<<subname<<"_static = this->"<<subname<<";"<<endl;
            pe.second->cppWriteSetStatement
                (
                  f, subname, subname, subname+"_static", ""
                  );
            f<<"}"<<endl;
          }
          f<<"}"<<endl;

          //from other ParameterSet into current static data
          f<<"void get(const insight::ParameterSet& p)"<<endl
          <<"{"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" "<<base_type_name<<"::get(p);"<<endl;
          }
          for ( const ParameterSetEntry& pe: result )
          {
            std::string subname=pe.first;
            f<<"{"<<endl;
            f<<"const "<<pe.second->cppParamType ( subname ) <<"& "<<subname<<" = p.get< "<<pe.second->cppParamType ( subname ) <<" >(\""<<subname<<"\");"<<endl;
            f<<pe.second->cppTypeName ( subname ) <<"& "<<subname<<"_static = this->"<<subname<<";"<<endl;
            pe.second->cppWriteGetStatement
                (
                  f, subname, subname, subname+"_static", ""
                  );
            f<<"}"<<endl;
          }
          f<<"}"<<endl;

          // set_variable initialization function
          for ( const ParameterSetEntry& pe: result )
          {
            std::string subname=pe.first;

            f<<name<<"& set_"<<subname<<"(const "<<pe.second->cppTypeName ( subname ) <<"& value)"<<endl
            <<"{"<<endl;
            f<<" this->"<<subname<<" = value;"<<endl;
            f<<" return *this;"<<endl;
            f<<"}"<<endl;
          }

          // create a ParameterSet with default values set
          f<<"static ParameterSet makeDefault() {"<<endl;
          f<<"ParameterSet p;"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" p="<<base_type_name<<"::makeDefault();"<<endl;
          }
          for ( const ParameterSetEntry& pe: result )
          {
            pe.second->cppWriteInsertStatement
                (
                  f,
                  "p",
                  pe.first
                  );
          }
          f<<"return p;"<<endl<<"}"<<endl;

          // convert static data into a ParameterSet
          f<<"virtual operator ParameterSet() const"<<endl;
          f<<"{ ParameterSet p=makeDefault(); set(p); return p; }"<<endl;

          f<<"};"<<endl;
        }
      }
    }
    catch( PDLException e )
    {
      std::cerr << "Error in processing PDL " << inf << "\n"
                                                        "Error Message:\n"
                << e.msg() << std::endl;
      return -1;
    }
    catch (...)
    {
      std::cerr << "Error in processing PDL " << inf << "\n" << std::endl;
      return -2;
    }
  }
  return 0;
}
