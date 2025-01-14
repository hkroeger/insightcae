#ifndef INTPARAMETERPARSER_H
#define INTPARAMETERPARSER_H

#include "parametergenerator.h"

struct IntParameterParser
    : public ParameterGenerator
{
        int value;

        IntParameterParser(int v, const std::string& d);

        void cppAddRequiredInclude(std::set<std::string>& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;

    declareType("int");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::int_ >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                         phx::new_<IntParameterParser>(qi::_1, qi::_2)) ]

            )
        );
    }
};

#endif // INTPARAMETERPARSER_H
