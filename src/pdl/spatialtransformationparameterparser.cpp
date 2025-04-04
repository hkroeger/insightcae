#include "spatialtransformationparameterparser.h"

using namespace std;

defineType(SpatialTransformationParameterParser);
addToStaticFunctionTable(ParameterGenerator, SpatialTransformationParameterParser, insertrule);


SpatialTransformationParameterParser
::SpatialTransformationParameterParser(
    const arma::mat& t,
    const arma::mat& r,
    double s,
    const std::string& d )
: ParameterGenerator(d), trans(t), rpy(r), scale(s)
{}


void SpatialTransformationParameterParser::cppAddRequiredInclude(std::set<std::string>& headers) const
{
  headers.insert("\"base/spatialtransformation.h\"");
  headers.insert("\"base/parameters/spatialtransformationparameter.h\"");
}

std::string SpatialTransformationParameterParser::cppInsightType() const
{
    return "insight::SpatialTransformationParameter";
}

std::string SpatialTransformationParameterParser::cppStaticType() const
{
  return "insight::SpatialTransformation";
}

std::string SpatialTransformationParameterParser::cppDefaultValueExpression() const
{
  std::ostringstream os;
  os << cppStaticType() << "(";
  writeVec3Constant(os, trans);
  os<<", ";
  writeVec3Constant(os, rpy);
  os<<", "
    << scale << ")";
  return os.str();
}
