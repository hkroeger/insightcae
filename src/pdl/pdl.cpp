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


#include "parserdatabase.h"


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



skip_grammar::skip_grammar()
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


PDLParserRuleset::PDLParserRuleset()
{  
  r_string = as_string[ lexeme [ "\"" >> *~char_("\"") >> "\"" ] ];
  r_description_string = (r_string | attr(""));
  r_identifier = lexeme[ alpha >> *(alnum | char_('_') | char_(':') ) >> !(alnum | '_' | ':') ];
  r_path = lexeme[ alpha >> *(alnum | char_('_') | char_('/') ) >> !(alnum | '_' | '/' ) ];
  r_up_to_semicolon = qi::as_string[ qi::lexeme [ *~(qi::char_(";")) >> ";" ] ];

  r_parameterdata %= omit[ parameterDataRules[ qi::_a = qi::_1 ] ] > qi::lazy(*qi::_a)
                  > -( qi::lit("*necessary") [ phx::bind(&ParserDataBase::setNecessary, *qi::_val) ] )
                  > -( qi::lit("*expert") [ phx::bind(&ParserDataBase::setExpert, *qi::_val) ] )
                  > -( qi::lit("*hidden") [ phx::bind(&ParserDataBase::setHidden, *qi::_val) ] )
      ;
  


  r_parametersetentry = r_identifier >> '=' > r_parameterdata;

  r_parameterset =  *( r_parametersetentry );

  r_addcode =
     qi::lit("addTo_makeDefault") >> as_string[ lexeme [ "{" >> *~char_("}") >> "}" ] ] [qi::_val = qi::_1]
    ;

  r_pdl_content =
         -( as_string[ lexeme [ "<" >> *~char_(">") >> ">" ] ] )
      >> -( qi::lit("inherits") >> r_identifier )
      >> -( qi::lit("description") >> r_string )
      >> ( ( qi::lit("skipDefaultParametersMember") >> qi::attr(true) ) | qi::attr(false) )
      >> (r_addcode | qi::attr(std::string()))
      >> r_parameterset;


}
/**
 * \addtogroup PDL
 * \subsection grammar_primitives Primitives
 * - Strings are any char confined inside quotes ""
 * - Descriptor strings may be omitted. In this case, an empty descriptor is inserted.
 * - Identifiers must not be enclosed by quotes but have to start with an alphabetical character.
 *   They may then contain alphanumerical chars or underscores.
 */



struct PDLParser
    : qi::grammar< PDLParserRuleset::Iterator, PDLParserResult(), PDLParserRuleset::Skipper >
{

  public:
  
  PDLParserRuleset rules;

  PDLParser()
      : PDLParser::base_type(rules.r_pdl_content)
  {

      for (const auto& r: *ParserDataBase::insertruleFunctions_)
      {
          r.second(rules);
      }

      rules.init();

      on_error<fail>( rules.r_pdl_content,
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

      PDLParser parser;
      skip_grammar skip;


      std::istreambuf_iterator<char> eos;
      std::string contents_raw ( std::istreambuf_iterator<char> ( in ), eos );

      std::string::iterator first=contents_raw.begin();
      std::string::iterator last=contents_raw.end();

      PDLParserResult result_all;
      if (!qi::phrase_parse
          (
            first,
            last,
            parser,
            skip,
            result_all
            ))
      {
        throw PDLException("Parsing PDL "+inf.string()+" failed!");
      }


      std::string templateArgs="";
      auto templateArgsP = boost::fusion::get<0>(result_all);
      if (templateArgsP) templateArgs=*templateArgsP;

      std::string default_base_type_name="insight::ParametersBase";
      std::string base_type_name=default_base_type_name;
      auto inheritParam = boost::fusion::get<1>(result_all);
      if (inheritParam) base_type_name=*inheritParam;

      std::string description="";
      auto descParam = boost::fusion::get<2>(result_all);
      if (descParam) description=*descParam;

      bool skipDefaultParamMember = boost::fusion::get<3>(result_all);

      std::string addTo_makeDefault = boost::fusion::get<4>(result_all);

      ParameterSetData result = boost::fusion::get<5>(result_all);

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
          if (base_type_name==default_base_type_name)
          {
            headers.insert("\"base/supplementedinputdata.h\"");
          }
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

          if (!templateArgs.empty())
            f<<"template<"<<templateArgs<<">\n";

          f<<"struct "<<name<<endl;
          if ( !base_type_name.empty() )
          {
            f<<": public "<<base_type_name<<endl;
          }
          f<<"{"<<endl;

          // declare variables and types
          for ( const ParameterSetEntry& pe: result )
          {
            pe.second->writeCppHeader ( f, pe.first, "" );
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
          f<<name<<"(const insight::SubsetParameter& p)"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" : "<<base_type_name<<"(p)"<<endl;
          }
          f <<"{"<<endl
           <<" get(p);"<<endl
          <<"}"<<endl
            ;

          //set into other ParameterSet
          f<<"void set(insight::SubsetParameter& p) const override"<<endl
          <<"{"<<endl;
          if ( !base_type_name.empty() && (base_type_name!=default_base_type_name) )
          {
            f<<" "<<base_type_name<<"::set(p);"<<endl;
          }
          for ( const ParameterSetEntry& pe: result )
          {
            std::string subname=pe.first;
            f<<"{"<<endl;
            f<<"auto& "<<subname<<" = p.get< "<<pe.second->cppParamType ( subname ) <<" >(\""<<subname<<"\");"<<endl;
            f<<"const auto& "<<subname<<"_static = this->"<<subname<<";"<<endl;
            pe.second->cppWriteSetStatement
                (
                  f, subname, subname, subname+"_static", ""
                  );
            f<<"}"<<endl;
          }
          f<<"}"<<endl;

          f<<"void get(const insight::SubsetParameter& p) override"<<endl
          <<"{"<<endl;
          if ( !base_type_name.empty() && (base_type_name!=default_base_type_name) )
          {
            f<<" "<<base_type_name<<"::get(p);"<<endl;
          }
          for ( const ParameterSetEntry& pe: result )
          {
            std::string subname=pe.first;
            f<<"{"<<endl;
            f<<"const auto& "<<subname<<" = p.get< "<<pe.second->cppParamType ( subname ) <<" >(\""<<subname<<"\");"<<endl;
            f<<"auto& "<<subname<<"_static = this->"<<subname<<";"<<endl;
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
          f<<"insight::ParameterSet p;"<<endl;
          if ( !base_type_name.empty() )
          {
            f<<" p="<<base_type_name<<"::makeDefault();"<<endl;
          }
          f<<" p.setParameterSetDescription(\""<<description<<"\");"<<endl;
          for ( const ParameterSetEntry& pe: result )
          {
            pe.second->cppWriteInsertStatement
                (
                  f,
                  "p",
                  pe.first,
                  ""
                  );
          }
          f<<addTo_makeDefault<<endl;
          f<<"return p;"<<endl<<"}"<<endl;

          // convert static data into a ParameterSet
          f<<"operator ParameterSet() const override"<<endl;
          f<<"{ insight::ParameterSet p=makeDefault(); set(p); return p; }"<<endl;

          // clone function
          f<<"std::unique_ptr<insight::ParametersBase> clone() const override"<<endl;
          f<<"{ return std::make_unique<"<<name<<">(*this); }"<<endl;

          f<<"};"<<endl;


          if (!skipDefaultParamMember)
            f << "static insight::ParameterSet defaultParameters() { return "<<name<<"::makeDefault(); }" << endl;
        }
      }
    }
    catch( const std::exception& e )
    {
      std::cerr << "Error in processing PDL: " << e.what() << "\n" << std::endl;
      return -1;
    }
  }
  return 0;
}
