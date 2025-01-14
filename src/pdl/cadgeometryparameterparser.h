#ifndef CADGEOMETRYPARAMETERPARSER_H
#define CADGEOMETRYPARAMETERPARSER_H


#include "parametergenerator.h"

struct CADGeometryGenerator
    : public ParameterGenerator
{
    std::string featureLabel_, script_;

    CADGeometryGenerator(
        const std::string& featlabel,
        const std::string& script,
        const std::string& d);

    void cppAddRequiredInclude(std::set< std::string >& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

    std::string cppConstructorParameters() const override;

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


  declareType("cadgeometry");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( ruleset.r_string >> ruleset.r_string >> ruleset.r_description_string )
                        [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                            phx::new_<CADGeometryGenerator>(qi::_1, qi::_2, qi::_3)) ]

      )
    );
  }
};

#endif // CADGEOMETRYPARAMETERPARSER_H
