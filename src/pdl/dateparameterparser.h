#ifndef DATEPARAMETERPARSER_H
#define DATEPARAMETERPARSER_H

#include "boost/date_time/gregorian/parsers.hpp"
#include "parserdatabase.h"

#include "boost/date_time/gregorian_calendar.hpp"

struct DateParameterParser
{
    struct Data
        : public ParserDataBase
    {
        boost::gregorian::date value;

        Data(boost::gregorian::date v, const std::string& d);

        void cppAddHeader(std::set<std::string>& headers) const override;

        std::string cppType(const std::string&) const override;

        std::string cppParamType(const std::string& ) const override;

        std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;
    };

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
                     qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(
                         phx::bind(&boost::gregorian::from_simple_string, qi::_1),
                         qi::_2)) ]

                    )
                );
    }
};

#endif // DATEPARAMETERPARSER_H
