#ifndef VECTORPARAMETERPARSER_H
#define VECTORPARAMETERPARSER_H

#include "parametergenerator.h"

struct VectorParameterParser
: public ParameterGenerator
{
    arma::mat value;
    enum VectorType { NonSpatial, Point, Direction };
    VectorType vectorType_;

    VectorParameterParser(const arma::mat& v, VectorType vt, const std::string& d);

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

              (
                  (
                      ( qi::lit("direction") > qi::attr(Direction) )
                      | ( qi::lit("point") > qi::attr(Point) )
                      | ( qi::attr(NonSpatial) )
                      ) >>
               "(" >> *qi::double_ >> ")" >>
                ruleset.r_description_string )
          [ qi::_val = phx::construct<ParameterGeneratorPtr>(
             phx::new_<VectorParameterParser>(
                           vec2mat_(qi::_2), qi::_1, qi::_3)
            ) ]

      )
    );
  }

};

#endif // VECTORPARAMETERPARSER_H
