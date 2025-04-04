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


#include "parametergenerator.h"

#include "parametersetgenerator.h"


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
  r_string = lexeme [ "\"" >> *~char_("\"") >> "\"" ];
  r_description_string = (r_string | attr(""));
  r_identifier = as_string [ lexeme[ alpha >> *(alnum|char_('_')|char_(':')|char_('<')|char_('>')) >> !(alnum | '_'| ':'|'<'|'>') ]];
  r_path = lexeme[ alpha >> *(alnum | char_('_') | char_('/') ) >> !(alnum | '_' | '/' ) ];
  r_up_to_semicolon = qi::as_string[ qi::lexeme [ *~(qi::char_(";")) >> ";" ] ];
  r_addcode = as_string[ lexeme [ '{' >> *~char_('}') >> '}' ] ];
  r_templateArg = lexeme [ '<' >> *~char_('>') >> '>' ];

  r_parameterdata %= omit[ parameterDataRules[ qi::_a = qi::_1  ] ] > qi::lazy(*qi::_a)
                  > -( qi::lit("*necessary") [ phx::bind(&ParameterGenerator::setNecessary, *qi::_val) ] )
                  > -( qi::lit("*expert") [ phx::bind(&ParameterGenerator::setExpert, *qi::_val) ] )
                  > -( qi::lit("*hidden") [ phx::bind(&ParameterGenerator::setHidden, *qi::_val) ] )
      ;
  


  r_parametersetentry = (r_identifier >> '=' >> r_parameterdata ) [
      phx::bind(&ParameterGenerator::setName, *qi::_2, qi::_1),
       qi::_val = qi::_2 ]
  ;

  r_parameterset = (
         -( r_templateArg )
      >> -( qi::lit("inherits") >> r_identifier )
      >> -( qi::lit("description") >> r_string )
      >> -( qi::lit("addTo_makeDefault") >> r_addcode )
      >> *( r_parametersetentry ) )
            [ qi::_val = phx::construct<SubsetGeneratorPtr>(
                phx::new_<SubsetGenerator>(
                  qi::_5, qi::_1, qi::_2, qi::_3, qi::_4)) ];

  r_pdl_content =
         ( ( qi::lit("skipDefaultParametersMember") >> qi::attr(true) ) | qi::attr(false) )
      >> r_parameterset
      >> ( ( qi::lit("createGetter") >> qi::attr(true) ) | qi::attr(false) )
      ;


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

      for (const auto& r: *ParameterGenerator::insertruleFunctions_)
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
      if ( !qi::phrase_parse(
            first, last,
            parser, skip,
            result_all ) )
      {
          throw PDLException("Parsing PDL "+inf.string()+" failed!");
      }

      if (last!=contents_raw.end())
      {
          throw PDLException(
              "Parsing PDL "+inf.string()+" incomplete! "
              "Remaining: "+std::string(last, contents_raw.end()) );
      }


      const std::string defaultParameterSetBaseClass = "insight::ParametersBase";
      bool skipDefaultParamMember = boost::fusion::get<0>(result_all);
      bool createGetter = boost::fusion::get<2>(result_all);

      {
        std::string bname=inf.stem().string();
        std::vector<std::string> parts;
        boost::algorithm::split_regex ( parts, bname, boost::regex ( "__" ) );
        std::string name=bname;
        if ( parts.size() ==3 )
        {
          name=parts[2];
        }

        std::set<std::string> headers;

        auto parameterSetData = std::make_shared<ParameterSetGenerator>(
            *boost::fusion::get<1>(result_all) );

        if (!parameterSetData->base_type_name_)
        {
            headers.insert("\"base/supplementedinputdata.h\"");
            parameterSetData->base_type_name_=defaultParameterSetBaseClass;
        }

        parameterSetData->setName(name);
        parameterSetData->setPath("");

        parameterSetData->cppAddRequiredInclude ( headers );

        {
          std::ofstream f ( bname+"_headers.h" );
          for ( auto& h: headers )
          {
            f<<"#include "<<h<<endl;
          }
        }


        {
          std::ofstream f ( bname+".h" );

          parameterSetData->writeCppTypeDecl(f);

          // ============================================================
          // some additions to the enclosing (user) class

          if (!skipDefaultParamMember)
            f << "static std::unique_ptr<insight::ParameterSet> defaultParameters()\n"
              << "{ return "<<name<<"::makeDefault(); }\n";

          if (createGetter)
          {
              if (parameterSetData->base_type_name_.value()
                  == defaultParameterSetBaseClass)
              {
                  f << "protected:\n"
                    <<  "ParameterSetInput p_;\n"
                    << "public:\n"
                    <<  "const ParameterSet& getParameters() const { return p_.parameterSet(); }\n"
                      ;
              }
              f << "public:\n"
                << "const " <<name<< "& p() const { return dynamic_cast<const " <<name<< "&>(this->p_.parameters()); }\n"
                // << name << "& p() { return dynamic_cast< " <<name<< "&>(this->p_.parameters()); }\n"
                  ;
          }
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
