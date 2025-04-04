#ifndef ARRAYGENERATOR_H
#define ARRAYGENERATOR_H

#include "parametergenerator.h"

struct ArrayGenerator : public ParameterGenerator
{
    ParameterGeneratorPtr value;
    size_t num;

    ArrayGenerator(ParameterGeneratorPtr v, size_t n, const std::string& d);

    void setName(const std::string& name) override;

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;

    void writeCppTypeDecl(
        std::ostream& os ) const override;


    void cppWriteCreateStatement
    (
        std::ostream& os,
        const std::string& psvarname
    ) const override;

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


  declareType("array");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
       typeName,
       std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( '[' >> ruleset.r_parameterdata >> ']' >> '*' >> qi::int_ >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParameterGeneratorPtr>(phx::new_<ArrayGenerator>(qi::_1, qi::_2, qi::_3)) ]

      )
    );
  }
};


#endif // ARRAYGENERATOR_H
