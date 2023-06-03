#ifndef SUBSETPARAMETERPARSER_H
#define SUBSETPARAMETERPARSER_H

#include "parserdatabase.h"

struct SubsetParameterParser
{

    struct Data
            : public ParserDataBase
    {
        ParameterSetData value;

        Data(const ParameterSetData& v, const std::string& d);

        void cppAddHeader(std::set< std::string >& headers) const override;

        std::string cppType(const std::string&) const override;

        std::string cppTypeDecl(const std::string& name,
                                const std::string& thisscope) const override;

        std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;

        std::string cppParamType(const std::string& ) const override;

        void cppWriteInsertStatement
        (
            std::ostream& os,
            const std::string& psvarname,
            const std::string& name,
            const std::string& thisscope
        ) const override;

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

    declareType("set");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( "{" > ruleset.r_parameterset > "}" >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]

            )
        );
    }
};


#endif // SUBSETPARAMETERPARSER_H
