#include "vectorparameterparser.h"

using namespace std;

VectorParameterParser::Data::Data(const arma::mat& v, const std::string& d)
: ParserDataBase(d), value(v)
{}

void VectorParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("<armadillo>");
}

std::string VectorParameterParser::Data::cppType(const std::string&) const
{
  return "arma::mat";
}

std::string VectorParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::VectorParameter";
}

std::string VectorParameterParser::Data::cppValueRep(const std::string& ) const
{
  std::ostringstream os;
  os<<"arma::mat(boost::assign::list_of";
  for (size_t i=0; i<value.n_elem; i++)
  {
    os<<"("<<value(i)<<")";
  }
  os<<".convert_to_container<std::vector<double> >().data(), "<<value.n_elem<<", 1)";
  return os.str();
}
