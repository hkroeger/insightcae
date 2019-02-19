#include "intparameterparser.h"

using namespace std;


IntParameterParser::Data::Data(int v, const std::string& d)
    : ParserDataBase(d), value(v)
{}

std::string IntParameterParser::Data::cppType(const std::string&) const
{
    return "int";
}

std::string IntParameterParser::Data::cppParamType(const std::string& ) const
{
    return "insight::IntParameter";
}

std::string IntParameterParser::Data::cppValueRep(const std::string& ) const
{
    return boost::lexical_cast<std::string>(value);
}
