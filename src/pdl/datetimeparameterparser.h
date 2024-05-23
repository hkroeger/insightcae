#ifndef DATETIMEPARAMETERPARSER_H
#define DATETIMEPARAMETERPARSER_H

#include "boost/date_time/posix_time/posix_time.hpp"
#include "parserdatabase.h"

struct DateTimeParameterParser
{
    struct Data
        : public ParserDataBase
    {
        boost::posix_time::ptime value;

        Data(boost::posix_time::ptime v, const std::string& d);

        void cppAddHeader(std::set<std::string>& headers) const override;

        std::string cppType(const std::string&) const override;

        std::string cppParamType(const std::string& ) const override;

        std::string cppValueRep(const std::string&, const std::string& thisscope ) const override;
    };

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
                         qi::_val = phx::construct<ParserDataBase::Ptr>(phx::new_<Data>(
                             phx::bind(&boost::posix_time::time_from_string, qi::_1),
                             qi::_2)) ]

                    )
                );
    }
};

#endif // DATETIMEPARAMETERPARSER_H
