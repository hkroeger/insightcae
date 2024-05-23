#include "dateparameterparser.h"

using namespace std;

defineType(DateParameterParser);
addToStaticFunctionTable(ParserDataBase, DateParameterParser, insertrule);

DateParameterParser::Data::Data(boost::gregorian::date v, const std::string& d)
    : ParserDataBase(d), value(v)
{}

void DateParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
    headers.insert("\"base/parameters/simpleparameter.h\"");
}


std::string DateParameterParser::Data::cppType(const std::string&) const
{
    return "boost::gregorian::date";
}

std::string DateParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::DateParameter";
}

std::string DateParameterParser::Data::cppValueRep(const std::string&, const std::string& thisscope ) const
{
    return str(boost::format("boost::gregorian::date{%d,%d,%d}")
               % value.year() % value.month().as_number() % value.day().as_number());
}
