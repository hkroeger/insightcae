#ifndef DATEPARAMETERPARSER_H
#define DATEPARAMETERPARSER_H

#include "boost/date_time/gregorian/parsers.hpp"
#include "parametergenerator.h"

#include "boost/date_time/gregorian_calendar.hpp"

struct DateGenerator
    : public ParameterGenerator
{
        boost::gregorian::date value;

        DateGenerator(boost::gregorian::date v, const std::string& d);

        void cppAddRequiredInclude(std::set<std::string>& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;

    declareType("date");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::as_string[qi::raw[ qi::int_ >> '-' >> qi::int_ >> '-' >> qi::int_]]
                   >> ruleset.r_description_string )
                    [ std::cout<<"'"<<qi::_1<<"'"<<std::endl,
                     qi::_val = phx::construct<ParameterGeneratorPtr>(
                        phx::new_<DateGenerator>(
                         phx::bind(&boost::gregorian::from_simple_string, qi::_1),
                         qi::_2)) ]

                    )
                );
    }
};

#endif // DATEPARAMETERPARSER_H
