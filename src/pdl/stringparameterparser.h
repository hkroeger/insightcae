#ifndef STRINGPARAMETERPARSER_H
#define STRINGPARAMETERPARSER_H

#include "parametergenerator.h"

struct StringGenerator
: public ParameterGenerator
{
    std::string value;

    StringGenerator(const std::string& v, const std::string& d);

    void cppAddRequiredInclude(std::set< std::string >& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;


  declareType("string");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( ruleset.r_string >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<StringGenerator>(qi::_1, qi::_2)) ]

      )
    );
  }
};

#endif // STRINGPARAMETERPARSER_H
