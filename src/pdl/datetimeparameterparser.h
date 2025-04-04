#ifndef DATETIMEPARAMETERPARSER_H
#define DATETIMEPARAMETERPARSER_H

#include "boost/date_time/posix_time/posix_time.hpp"
#include "parametergenerator.h"

struct DateTimeGenerator
    : public ParameterGenerator
{
        boost::posix_time::ptime value;

        DateTimeGenerator(boost::posix_time::ptime v, const std::string& d);

        void cppAddRequiredInclude(std::set<std::string>& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;

    declareType("datetime");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(

                    ( qi::as_string[ qi::raw[
                     qi::int_ >> '-' >> qi::int_ >> '-' >> qi::int_ >> qi::int_ >> ':' >> qi::int_
                      ] ] >> ruleset.r_description_string )
                        [ std::cout<<"'"<<qi::_1<<"'"<<std::endl,
                         qi::_val = phx::construct<ParameterGeneratorPtr>(
                          phx::new_<DateTimeGenerator>(
                             phx::bind(&boost::posix_time::time_from_string, qi::_1),
                             qi::_2)) ]

                    )
                );
    }
};

#endif // DATETIMEPARAMETERPARSER_H
