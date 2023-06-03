#ifndef INTPARAMETERPARSER_H
#define INTPARAMETERPARSER_H

#include "parserdatabase.h"

struct IntParameterParser
{
    struct Data
            : public ParserDataBase
    {
        int value;

        Data(int v, const std::string& d);

        void cppAddHeader(std::set<std::string>& headers) const override;

        std::string cppType(const std::string&) const override;

        std::string cppParamType(const std::string& ) const override;

        std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;
    };

    declareType("int");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::int_ >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(qi::_1, qi::_2)) ]

            )
        );
    }
};

#endif // INTPARAMETERPARSER_H
