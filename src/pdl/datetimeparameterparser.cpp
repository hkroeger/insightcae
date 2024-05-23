#include "datetimeparameterparser.h"

using namespace std;

defineType(DateTimeParameterParser);
addToStaticFunctionTable(ParserDataBase, DateTimeParameterParser, insertrule);

DateTimeParameterParser::Data::Data(boost::posix_time::ptime v, const std::string& d)
    : ParserDataBase(d), value(v)
{}

void DateTimeParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
    headers.insert("\"base/parameters/simpleparameter.h\"");
}


std::string DateTimeParameterParser::Data::cppType(const std::string&) const
{
    return "boost::posix_time::ptime";
}

std::string DateTimeParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::DateTimeParameter";
}

std::string DateTimeParameterParser::Data::cppValueRep(const std::string&, const std::string& thisscope ) const
{
    return str(boost::format("boost::posix_time::ptime{ boost::gregorian::date{%d,%d,%d}, boost::posix_time::time_duration{%d, %d, 0, 0} }")
               % value.date().year() % value.date().month().as_number() % value.date().day().as_number()
               % value.time_of_day().hours() % value.time_of_day().minutes() );
}
