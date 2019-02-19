#include "doubleparameterparser.h"


using namespace std;

DoubleParameterParser::Data::Data(double v, const std::string& d)
: ParserDataBase(d), value(v)
{}

std::string DoubleParameterParser::Data::cppType(const std::string&) const
{
  return "double";
}

std::string DoubleParameterParser::Data::cppParamType(const std::string&) const
{
  return "insight::DoubleParameter";
}

std::string DoubleParameterParser::Data::cppValueRep(const std::string&) const
{
  return boost::lexical_cast<std::string>(value);
}
