#ifndef OPENFOAMCASEDIRECTORYGENERATOR_H
#define OPENFOAMCASEDIRECTORYGENERATOR_H


#include "directorygenerator.h"

struct OpenFOAMCaseDirectoryGenerator
    : public DirectoryGenerator
{
        OpenFOAMCaseDirectoryGenerator(const boost::filesystem::path& v, const std::string& d);

        void cppAddRequiredInclude(std::set< std::string >& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;

        void cppWriteGetStatement
            (
                std::ostream& os,
                const std::string& varname,
                const std::string& staticname
                ) const override;

    declareType("openfoamCaseDirectory");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                    ( ruleset.r_string >> ruleset.r_description_string )
                        [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                          phx::new_<OpenFOAMCaseDirectoryGenerator>(qi::_1, qi::_2)) ]

                    )
                );
    }
};

#endif // OPENFOAMCASEDIRECTORYGENERATOR_H
