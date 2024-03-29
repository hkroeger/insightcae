#ifndef SPATIALTRANSFORMATIONPARAMETERPARSER_H
#define SPATIALTRANSFORMATIONPARAMETERPARSER_H

#include "parserdatabase.h"

struct SpatialTransformationParameterParser
{
  struct Data
  : public ParserDataBase
  {
    arma::mat trans, rpy;
    double scale;

    Data(const arma::mat& trans, const arma::mat& rpy, double scale, const std::string& d);

    void cppAddHeader(std::set<std::string>& headers) const override;

    std::string cppType(const std::string&) const override;

    std::string cppParamType(const std::string&) const override;

    std::string cppValueRep(const std::string&, const std::string& thisscope) const override;
  };

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
         [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(
            vec2mat_(qi::_1), vec2mat_(qi::_2), qi::_3, qi::_4 )) ]

      )
    );
  }
};

#endif // SPATIALTRANSFORMATIONPARAMETERPARSER_H
