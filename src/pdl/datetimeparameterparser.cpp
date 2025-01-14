#include "datetimeparameterparser.h"

using namespace std;

defineType(DateTimeGenerator);
addToStaticFunctionTable(ParameterGenerator, DateTimeGenerator, insertrule);

DateTimeGenerator::DateTimeGenerator(boost::posix_time::ptime v, const std::string& d)
    : ParameterGenerator(d), value(v)
{}


void DateTimeGenerator::cppAddRequiredInclude(std::set<std::string>& headers) const
{
    headers.insert("\"base/parameters/simpleparameter.h\"");
}

std::string DateTimeGenerator::cppInsightType() const
{
    return "insight::DateTimeParameter";
}

std::string DateTimeGenerator::cppStaticType() const
{
    return "boost::posix_time::ptime";
}


std::string DateTimeGenerator::cppDefaultValueExpression() const
{
    return str(boost::format("boost::posix_time::ptime{ boost::gregorian::date{%d,%d,%d}, boost::posix_time::time_duration{%d, %d, 0, 0} }")
               % value.date().year() % value.date().month().as_number() % value.date().day().as_number()
               % value.time_of_day().hours() % value.time_of_day().minutes() );
}
