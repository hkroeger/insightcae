#include "vectorparameterparser.h"
#include "boost/lexical_cast.hpp"

using namespace std;

defineType(VectorParameterParser);
addToStaticFunctionTable(ParameterGenerator, VectorParameterParser, insertrule);


VectorParameterParser::VectorParameterParser(
    const arma::mat& v, const std::string& d)
: ParameterGenerator(d), value(v)
{}

void VectorParameterParser::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("<armadillo>");
  headers.insert("\"base/parameters/simpleparameter.h\"");
}

std::string VectorParameterParser::cppInsightType() const
{
    return "insight::VectorParameter";
}

std::string VectorParameterParser::cppStaticType() const
{
  return "arma::mat";
}

std::string VectorParameterParser::cppDefaultValueExpression() const
{
    std::vector<std::string> defCmpts;
    for (size_t i=0; i<value.n_elem; i++)
    {
        defCmpts.push_back(
            boost::lexical_cast<std::string>(
                value(i) ) );
    }

    return "arma::mat({"+boost::join(defCmpts, ", ")+"}).t()";
}
