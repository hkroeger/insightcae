#ifndef INCLUDEDSUBSETPARAMETERPARSER_H
#define INCLUDEDSUBSETPARAMETERPARSER_H


#include "parametergenerator.h"


struct IncludedSubsetGenerator
    : public ParameterGenerator
{
        std::string value;

        typedef boost::fusion::vector3<std::string, std::string, std::string> DefaultModification;
        typedef std::vector<DefaultModification> DefaultValueModifications;

        DefaultValueModifications default_value_modifications;

        IncludedSubsetGenerator(
            const std::string& v,
            const std::string& d,
            const DefaultValueModifications& defmod );

        // void setPath(const std::string &containerPath) override;

        void cppAddRequiredInclude(std::set<std::string>& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;


        void cppWriteCreateStatement
        (
            std::ostream& os,
            const std::string& psvarname
        ) const override;

        void cppWriteInsertStatement
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


    declareType("includedset");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                (ruleset.r_string >> ruleset.r_description_string >>
                 (
                   (qi::lit("modifyDefaults") >> '{' >>
                    *(
                      ruleset.r_identifier >> ruleset.r_path
                        >> '=' >> ruleset.r_up_to_semicolon
                     )
                   >> '}')
                   |qi::attr(IncludedSubsetGenerator::DefaultValueModifications())
                  ) )
                [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                         phx::new_<IncludedSubsetGenerator>(
                             qi::_1, qi::_2, qi::_3)) ]

            )
        );
    }
};


#endif // INCLUDEDSUBSETPARAMETERPARSER_H
