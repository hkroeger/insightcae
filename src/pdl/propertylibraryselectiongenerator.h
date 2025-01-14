#ifndef PROPERTYLIBRARYSELECTIONGENERATOR_H
#define PROPERTYLIBRARYSELECTIONGENERATOR_H


#include "parametergenerator.h"

struct PropertyLibrarySelectionGenerator
: public ParameterGenerator
{
    bool isTemplate;
    std::string libraryName;
    std::string defaultSelection;

    PropertyLibrarySelectionGenerator(bool isTemplate, const std::string& libname, const std::string& sel, const std::string& d);

    bool isPrimitiveType() const override;

    void cppAddRequiredInclude(std::set<std::string>& headers) const override;

    std::string cppInsightType() const override;
    std::string cppStaticType() const override;
    std::string cppDefaultValueExpression() const override;


    void cppWriteCreateStatement(
        std::ostream& os,
        const std::string& psvarname
    ) const override;

    void cppWriteSetStatement(
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
    ) const override;

    void cppWriteGetStatement(
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
    ) const override;


  declareType("librarySelection");

  inline static void insertrule(PDLParserRuleset& ruleset)
  {
    ruleset.parameterDataRules.add
    (
      typeName,
      std::make_shared<PDLParserRuleset::ParameterDataRule>(

            ( ( (qi::lit("template")>>qi::attr(true)) | qi::attr(false) )
               >> ruleset.r_string
               >> ruleset.r_identifier
               >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                       phx::new_<PropertyLibrarySelectionGenerator>(qi::_1, qi::_2, qi::_3, qi::_4)) ]

      )

    );
  }
};



#endif // PROPERTYLIBRARYSELECTIONGENERATOR_H
