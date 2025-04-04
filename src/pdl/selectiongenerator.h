#ifndef SELECTIONGENERATOR_H
#define SELECTIONGENERATOR_H

#include "parametergenerator.h"

struct SelectionGenerator
: public ParameterGenerator
{
    std::vector<std::string> selections;
    std::string selection;

    SelectionGenerator(
        const std::vector<std::string>& sels,
        const std::string& sel, const std::string& d);

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;


    void cppWriteCreateStatement(
        std::ostream& os,
        const std::string& psvarname ) const override;

    void cppWriteSetStatement(
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
    ) const override;

    void cppWriteGetStatement(
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname ) const override;

    // void cppWriteGetStatement(std::ostream& os, const std::string& name, const std::string& varname, const std::string& staticname,
    //   const std::string& thisscope) const override;


  declareType("selection");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

        ( "(" >> *(ruleset.r_identifier) >> ")" >> ruleset.r_identifier >> ruleset.r_description_string )
        [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<SelectionGenerator>(qi::_1, qi::_2, qi::_3)) ]

      )
    );
  }
};

#endif // SELECTIONGENERATOR_H
