#ifndef BOOLPARAMETERPARSER_H
#define BOOLPARAMETERPARSER_H

#include "parserdatabase.h"

struct BoolParameterParser
{
  struct Data
  : public ParserDataBase
  {
    bool value;

    Data(bool v, const std::string& d);

    void cppAddHeader(std::set<std::string>& headers) const override;

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string&) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope) const override;
  };

  declareType("bool");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( qi::bool_ >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]

      )
    );
  }

};

#endif // BOOLPARAMETERPARSER_H
