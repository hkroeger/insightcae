#include "intparameterparser.h"




using namespace std;




defineType(IntParameterParser);
addToStaticFunctionTable(ParameterGenerator, IntParameterParser, insertrule);




IntParameterParser::IntParameterParser(int v, const std::string& d)
    : ParameterGenerator(d), value(v)
{}




void IntParameterParser
    ::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("\"base/parameters/simpleparameter.h\"");
}




std::string IntParameterParser::cppStaticType() const
{
    return "int";
}



std::string IntParameterParser::cppInsightType() const
{
    return "insight::IntParameter";
}




std::string IntParameterParser::cppDefaultValueExpression() const
{
    return boost::lexical_cast<std::string>(value);
}
