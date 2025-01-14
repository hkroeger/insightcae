#include "doubleparameterparser.h"


using namespace std;

defineType(DoubleGenerator);
addToStaticFunctionTable(ParameterGenerator, DoubleGenerator, insertrule);



DoubleGenerator::DoubleGenerator(double v, const std::string& d)
: ParameterGenerator(d), value(v)
{}

void DoubleGenerator::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("\"base/parameters/simpleparameter.h\"");
}

std::string DoubleGenerator::cppInsightType() const
{
    return "insight::DoubleParameter";
}

std::string DoubleGenerator::cppStaticType() const
{
  return "double";
}

std::string DoubleGenerator::cppDefaultValueExpression() const
{
  return boost::lexical_cast<std::string>(value);
}



defineType(dimensionedScalarGenerator);
addToStaticFunctionTable(ParameterGenerator, dimensionedScalarGenerator, insertrule);


dimensionedScalarGenerator::dimensionedScalarGenerator
(
    const std::string& dimensionTypeName,
    const std::string& defaultUnit,
    double v,
    const std::string& d
)
: ParameterGenerator("["+defaultUnit+"] "+d),
  dimensionTypeName_(dimensionTypeName),
  defaultUnit_(defaultUnit),
  value(v)
{}


std::string dimensionedScalarGenerator::cppInsightType() const
{
    return "insight::scalar"+dimensionTypeName_+"Parameter";
}

std::string dimensionedScalarGenerator::cppStaticType() const
{
  return cppInsightType()+"::value_type";
}

std::string dimensionedScalarGenerator::cppDefaultValueExpression() const
{
  return cppStaticType()+"("+cppInsightType()+"::base_value_type("+boost::lexical_cast<std::string>(value)+") * boost::units::si::"+defaultUnit_+")";
}

