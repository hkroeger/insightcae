#include "doublerangegenerator.h"

using namespace std;

defineType(DoubleRangeGenerator);
addToStaticFunctionTable(ParameterGenerator, DoubleRangeGenerator, insertrule);

DoubleRangeGenerator::DoubleRangeGenerator(
    const std::vector<double>& v, const std::string& d)
: ParameterGenerator(d), value(v)
{}


void DoubleRangeGenerator::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("<set>");
  headers.insert("\"base/parameters/doublerangeparameter.h\"");
}

std::string DoubleRangeGenerator::cppInsightType() const
{
  return "insight::DoubleRangeParameter";
}

std::string DoubleRangeGenerator::cppStaticType() const
{
    return "std::set<double>";
}

std::string DoubleRangeGenerator::cppDefaultValueExpression() const
{
      std::ostringstream os;
      os<<"insight::DoubleRangeParameter::RangeList{";
      for (size_t i=0; i<value.size(); i++)
      {
          if (i>0) os<<", ";
          os<<value[i];
      }
      os<<"}";
      return os.str();
}

void DoubleRangeGenerator::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os<<varname<<".resetValues("<<staticname<<");"<<endl;
}

void DoubleRangeGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os << staticname<< " = "<<varname<<".values();\n"
       << staticname<< ".setPath( "<<varname<<" .path());\n" ;
}
