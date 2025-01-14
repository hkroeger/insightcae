#ifndef DOUBLERANGEGENERATOR_H
#define DOUBLERANGEGENERATOR_H

#include "parametergenerator.h"

struct DoubleRangeGenerator
: public ParameterGenerator
{
    std::vector<double> value;

    DoubleRangeGenerator(const std::vector<double>& v, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

    void cppWriteSetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
    ) const override;

    void cppWriteGetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
    ) const override;


  declareType("doubleRange");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

            ( "(" >> *qi::double_ >> ")" >> ruleset.r_description_string )
            [ qi::_val = phx::construct<ParameterGeneratorPtr>(
               phx::new_<DoubleRangeGenerator>(qi::_1, qi::_2)
              ) ]

      )
    );
  }
};



#endif // DOUBLERANGEGENERATOR_H
