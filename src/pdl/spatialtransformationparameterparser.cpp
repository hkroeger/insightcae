#include "spatialtransformationparameterparser.h"

using namespace std;

SpatialTransformationParameterParser::Data::Data(
        const arma::mat& t, const arma::mat& r, double s, const std::string& d )
: ParserDataBase(d), trans(t), rpy(r), scale(s)
{}

void SpatialTransformationParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("\"base/spatialtransformation.h\"");
  headers.insert("\"base/parameters/spatialtransformationparameter.h\"");
}

std::string SpatialTransformationParameterParser::Data::cppType(const std::string&) const
{
  return "insight::SpatialTransformation";
}

std::string SpatialTransformationParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::SpatialTransformationParameter";
}

std::string SpatialTransformationParameterParser::Data::cppValueRep(
        const std::string& name, const std::string& thisscope ) const
{
  std::ostringstream os;
  os << cppType(name) << "(";
  writeVec3Constant(os, trans);
  os<<", ";
  writeVec3Constant(os, rpy);
  os<<", "
    << scale << ")";
  return os.str();
}
