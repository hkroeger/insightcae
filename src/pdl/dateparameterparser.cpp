#include "dateparameterparser.h"

using namespace std;

defineType(DateGenerator);
addToStaticFunctionTable(ParameterGenerator, DateGenerator, insertrule);

DateGenerator::DateGenerator(boost::gregorian::date v, const std::string& d)
    : ParameterGenerator(d), value(v)
{}

void DateGenerator::cppAddRequiredInclude(std::set<std::string>& headers) const
{
    headers.insert("\"base/parameters/simpleparameter.h\"");
}

std::string DateGenerator::cppInsightType() const
{
    return "insight::DateParameter";
}

std::string DateGenerator::cppStaticType() const
{
    return "boost::gregorian::date";
}

std::string DateGenerator::cppDefaultValueExpression() const
{
    return str(boost::format("boost::gregorian::date{%d,%d,%d}")
               % value.year() % value.month().as_number() % value.day().as_number());
}
