#include "stringparameterparser.h"

using namespace std;

defineType(StringGenerator);
addToStaticFunctionTable(ParameterGenerator, StringGenerator, insertrule);


StringGenerator::StringGenerator(const std::string& v, const std::string& d)
: ParameterGenerator(d), value(v)
{}


void StringGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
  headers.insert("<string>");
  headers.insert("\"base/parameters/simpleparameter.h\"");
}

std::string StringGenerator::cppInsightType() const
{
    return "insight::StringParameter";
}

std::string StringGenerator::cppStaticType() const
{
  return "std::string";
}

std::string StringGenerator::cppDefaultValueExpression() const
{
  return "\""+value+"\"";
}
