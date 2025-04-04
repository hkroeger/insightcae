#ifndef VECTORPARAMETERPARSER_H
#define VECTORPARAMETERPARSER_H

#include "parametergenerator.h"

struct VectorParameterParser
: public ParameterGenerator
{
    arma::mat value;

    VectorParameterParser(const arma::mat& v, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;


  declareType("vector");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( "(" >> *qi::double_ >> ")" >> ruleset.r_description_string )
          [ qi::_val = phx::construct<ParameterGeneratorPtr>(
             phx::new_<VectorParameterParser>(
                           vec2mat_(qi::_1), qi::_2)
            ) ]

      )
    );
  }

};

#endif // VECTORPARAMETERPARSER_H
