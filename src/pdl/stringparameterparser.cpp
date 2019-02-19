#include "stringparameterparser.h"

using namespace std;

StringParameterParser::Data::Data(const std::string& v, const std::string& d)
: ParserDataBase(d), value(v)
{}

void StringParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
  headers.insert("<string>");
}

std::string StringParameterParser::Data::cppType(const std::string&) const
{
  return "std::string";
}

std::string StringParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::StringParameter";
}

std::string StringParameterParser::Data::cppValueRep(const std::string& ) const
{
  return "\""+boost::lexical_cast<std::string>(value)+"\"";
}
