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

std::string DoubleParameterParser::Data::cppValueRep(const std::string&, const std::string& /*thisscope*/) const
{
  return boost::lexical_cast<std::string>(value);
}




dimensionedScalarParameterParser::Data::Data
(
    const std::string& dimensionTypeName,
    const std::string& defaultUnit,
    double v,
    const std::string& d
)
: ParserDataBase("["+defaultUnit+"] "+d),
  dimensionTypeName_(dimensionTypeName),
  defaultUnit_(defaultUnit),
  value(v)
{}

std::string dimensionedScalarParameterParser::Data::cppType(const std::string&name) const
{
  return cppParamType(name)+"::value_type";
}

std::string dimensionedScalarParameterParser::Data::cppParamType(const std::string&) const
{
  return "insight::scalar"+dimensionTypeName_+"Parameter";
}

std::string dimensionedScalarParameterParser::Data::cppValueRep(const std::string&name, const std::string& /*thisscope*/) const
{
  return cppType(name)+"::from_value("+cppParamType(name)+"::base_value_type("+boost::lexical_cast<std::string>(value)+") * boost::units::si::"+defaultUnit_+".value())";
}

void dimensionedScalarParameterParser::Data::cppWriteCreateStatement
(
   std::ostream& os,
   const std::string& name,
   const std::string& /*thisscope*/
) const
{
   os<<"std::unique_ptr< "<<cppParamType(name)<<" > "<<name<<"("
     "new "<<cppParamType(name)<<"("
     <<boost::lexical_cast<std::string>(value)<<", "
     <<"boost::units::si::"+defaultUnit_<<", "
     <<"\""<<description<<"\", "
     << (isHidden?"true":"false")<<", "
     << (isExpert?"true":"false")<<", "
     << (isNecessary?"true":"false")<<", "
     << order
     <<")"
     "); ";
}
