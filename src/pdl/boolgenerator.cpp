#include "boolgenerator.h"

using namespace std;



defineType(BoolGenerator);
addToStaticFunctionTable(ParameterGenerator, BoolGenerator, insertrule);

BoolGenerator::BoolGenerator(bool v, const std::string& d)
: ParameterGenerator(d), value(v)
{}

void BoolGenerator::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("\"base/parameters/simpleparameter.h\"");
}

std::string BoolGenerator::cppInsightType() const
{
    return "insight::BoolParameter";
}

std::string BoolGenerator::cppStaticType() const
{
  return "bool";
}

std::string BoolGenerator::cppDefaultValueExpression() const
{
  return boost::lexical_cast<std::string>(value);
}
