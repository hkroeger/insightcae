#ifndef CADSKETCHPARAMETERPARSER_H
#define CADSKETCHPARAMETERPARSER_H


#include "parserdatabase.h"

struct CADSketchParameterParser
{
    struct Data
        : public ParserDataBase
    {
        std::string script_, makeDefaultParametersFunctionName_;

        Data(const std::string& script, const std::string& makeDefaultParametersFunctionName, const std::string& d);

        void cppAddHeader(std::set< std::string >& headers) const override;

        std::string cppType(const std::string&) const override;

        std::string cppParamType(const std::string& ) const override;

        std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;

        std::string cppConstructorParameters(const std::string &name,
                                             const std::string& thisscope) const override;

        void cppWriteSetStatement
            (
                std::ostream& os,
                const std::string&,
                const std::string& varname,
                const std::string& staticname,
                const std::string&
                ) const override;


        void cppWriteGetStatement
            (
                std::ostream& os,
                const std::string&,
                const std::string& varname,
                const std::string& staticname,
                const std::string&
                ) const override;

    };

    declareType("cadsketch");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                        ( ruleset.r_string >> ruleset.r_string >> ruleset.r_description_string )
                        [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2, qi::_3)) ]

                        )
                );
    }
};

#endif // CADSKETCHPARAMETERPARSER_H
