#ifndef PATHPARAMETERPARSER_H
#define PATHPARAMETERPARSER_H



#include "parametergenerator.h"

struct PathGenerator
    : public ParameterGenerator
{
    boost::filesystem::path value;

    PathGenerator(const boost::filesystem::path& v, const std::string& d);

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


  declareType("path");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( ruleset.r_string >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<PathGenerator>(qi::_1, qi::_2)) ]

      )
    );
  }
};

#endif // PATHPARAMETERPARSER_H
