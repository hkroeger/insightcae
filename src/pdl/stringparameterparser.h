#ifndef STRINGPARAMETERPARSER_H
#define STRINGPARAMETERPARSER_H

#include "parserdatabase.h"

struct StringParameterParser
{
  struct Data
  : public ParserDataBase
  {
    std::string value;

    Data(const std::string& v, const std::string& d);

    void cppAddHeader(std::set< std::string >& headers) const override;

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string& ) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;

  };

  declareType("string");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( ruleset.r_string >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]

      )
    );
  }
};

#endif // STRINGPARAMETERPARSER_H
