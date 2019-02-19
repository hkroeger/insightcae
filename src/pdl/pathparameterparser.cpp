#include "pathparameterparser.h"


PathParameterParser::Data::Data(const boost::filesystem::path& v, const std::string& d)
: ParserDataBase(d), value(v)
{}

void PathParameterParser::Data::cppAddHeader(std::set< std::string >& headers) const
{
  headers.insert("<boost/filesystem.hpp>");
}

std::string PathParameterParser::Data::cppType(const std::string&) const
{
  return "boost::filesystem::path";
}

std::string PathParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::PathParameter";
}

std::string PathParameterParser::Data::cppValueRep(const std::string& ) const
{
  return boost::lexical_cast<std::string>(value);
}
