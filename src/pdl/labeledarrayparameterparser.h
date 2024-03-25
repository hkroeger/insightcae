#ifndef LABELEDARRAYPARAMETERPARSER_H
#define LABELEDARRAYPARAMETERPARSER_H

#include "parserdatabase.h"


struct LabeledArrayParameterParser {

    struct Data
        : public ParserDataBase
    {
        std::string patternOrKeysPath;
        enum ArgType { Pattern, KeysPath} argType;

        ParserDataBase::Ptr value;
        size_t num;

        Data ( const std::string& pok, ArgType at, ParserDataBase::Ptr v, size_t n, const std::string& d );

        void cppAddHeader(std::set<std::string>& headers) const override;

        std::string cppValueRep(const std::string& name, const std::string& thisscope) const override;

        std::string cppType(const std::string& name) const override;

        std::string cppTypeDecl ( const std::string& name,
                                const std::string& thisscope ) const override;

        std::string cppParamType(const std::string& ) const override;

        void cppWriteCreateStatement
            (
                std::ostream& os,
                const std::string& name,
                const std::string& thisscope
                ) const override;

        void cppWriteSetStatement
            (
                std::ostream& os,
                const std::string& name,
                const std::string& varname,
                const std::string& staticname,
                const std::string& thisscope
                ) const override;

        void cppWriteGetStatement
            (
                std::ostream& os,
                const std::string& name,
                const std::string& varname,
                const std::string& staticname,
                const std::string& thisscope
                ) const override;
    };

    declareType("labeledarray");

    inline static void insertrule ( PDLParserRuleset& ruleset )
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::lit("keysFrom") >> ruleset.r_string
                 >> '[' >> ruleset.r_parameterdata >> ']'
                 >> '*' >> qi::int_ >> ruleset.r_description_string )
                    [ qi::_val = phx::construct<ParserDataBase::Ptr>(
                         phx::new_<Data>(qi::_1, Data::KeysPath, qi::_2, qi::_3, qi::_4)) ]
                |
                ( ruleset.r_string
                 >> '[' >> ruleset.r_parameterdata >> ']'
                 >> '*' >> qi::int_ >> ruleset.r_description_string )
                    [ qi::_val = phx::construct<ParserDataBase::Ptr>(
                         phx::new_<Data>(qi::_1, Data::Pattern, qi::_2, qi::_3, qi::_4)) ]
                )
            );
    }
};
#endif // LABELEDARRAYPARAMETERPARSER_H
