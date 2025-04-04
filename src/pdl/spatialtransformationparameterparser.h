#ifndef SPATIALTRANSFORMATIONPARAMETERPARSER_H
#define SPATIALTRANSFORMATIONPARAMETERPARSER_H

#include "parametergenerator.h"

struct SpatialTransformationParameterParser
: public ParameterGenerator
{
    arma::mat trans, rpy;
    double scale;

    SpatialTransformationParameterParser(
        const arma::mat& trans, const arma::mat& rpy, double scale, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;


  declareType("spatialTransformation");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( "(" >> *qi::double_ >> ")" >> // translation
          "(" >> *qi::double_ >> ")" >> // roll pitch yaw
          qi::double_ >>                // scale factor
          ruleset.r_description_string )
         [ qi::_val = phx::construct<ParameterGeneratorPtr>(
            phx::new_<SpatialTransformationParameterParser>(
                vec2mat_(qi::_1), vec2mat_(qi::_2), qi::_3, qi::_4 )) ]

      )
    );
  }
};

#endif // SPATIALTRANSFORMATIONPARAMETERPARSER_H
