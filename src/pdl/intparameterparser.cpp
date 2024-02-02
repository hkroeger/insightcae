#include "intparameterparser.h"

using namespace std;

defineType(IntParameterParser);
addToStaticFunctionTable(ParserDataBase, IntParameterParser, insertrule);

IntParameterParser::Data::Data(int v, const std::string& d)
    : ParserDataBase(d), value(v)
{}

void IntParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("\"base/parameters/simpleparameter.h\"");
}


std::string IntParameterParser::Data::cppType(const std::string&) const
{
    return "int";
}

std::string IntParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::IntParameter";
}

std::string IntParameterParser::Data::cppValueRep(const std::string&, const std::string& thisscope ) const
{
    return boost::lexical_cast<std::string>(value);
}
