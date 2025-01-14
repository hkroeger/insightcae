#ifndef BOOLGENERATOR_H
#define BOOLGENERATOR_H

#include "parametergenerator.h"

struct BoolGenerator
    : public ParameterGenerator
{
    bool value;

    BoolGenerator(bool v, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;


  declareType("bool");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( qi::bool_ >> ruleset.r_description_string )
            [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<BoolGenerator>(qi::_1, qi::_2)) ]

      )
    );
  }

};

#endif // BOOLGENERATOR_H
