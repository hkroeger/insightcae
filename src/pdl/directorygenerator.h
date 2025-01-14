#ifndef DIRECTORYGENERATOR_H
#define DIRECTORYGENERATOR_H


#include "parametergenerator.h"

struct DirectoryGenerator
    : public ParameterGenerator
{
        boost::filesystem::path value;

        DirectoryGenerator(const boost::filesystem::path& v, const std::string& d);

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

    declareType("directory");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                    ( ruleset.r_string >> ruleset.r_description_string )
                        [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                          phx::new_<DirectoryGenerator>(qi::_1, qi::_2)) ]

                    )
                );
    }
};

#endif // DIRECTORYGENERATOR_H
