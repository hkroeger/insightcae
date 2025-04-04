#ifndef DOUBLEPARAMETERPARSER_H
#define DOUBLEPARAMETERPARSER_H

#include "parametergenerator.h"

struct DoubleGenerator
: public ParameterGenerator
{
    double value;

    DoubleGenerator(double v, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

  declareType("double");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( qi::double_ >> ruleset.r_description_string )
         [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<DoubleGenerator>(qi::_1, qi::_2)) ]

      )
    );
  }
};



struct dimensionedScalarGenerator
: public ParameterGenerator
{
    std::string dimensionTypeName_, defaultUnit_;
    double value;

    dimensionedScalarGenerator(
        const std::string& dimensionTypeName, const std::string& defaultUnit, double v, const std::string& d);

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

  declareType("dimensionedScalar");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( ruleset.r_identifier >> ruleset.r_identifier >> qi::double_ >> ruleset.r_description_string )
         [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<dimensionedScalarGenerator>(qi::_1, qi::_2, qi::_3, qi::_4)) ]

      )
    );
  }
};

#endif // DOUBLEPARAMETERPARSER_H
