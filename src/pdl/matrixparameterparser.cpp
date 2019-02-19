#include "matrixparameterparser.h"

using namespace std;

MatrixParameterParser::Data::Data(arma::uword r, arma::uword c, const std::string& d)
: ParserDataBase(d), value(arma::zeros(r,c))
{}

void MatrixParameterParser::Data::cppAddHeader(std::set<std::string>& headers) const
{
  headers.insert("<armadillo>");
}

std::string MatrixParameterParser::Data::cppType(const std::string&) const
{
  return "arma::mat";
}

std::string MatrixParameterParser::Data::cppParamType(const std::string& ) const
{
  return "insight::MatrixParameter";
}

std::string MatrixParameterParser::Data::cppValueRep(const std::string& ) const
{
  return "#error";
}

void MatrixParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os, const std::string& name
) const
{

  os<<"std::auto_ptr< "<<cppParamType(name)<<" > "<<name<<";"<<endl;
//       os<<cppParamType(name)<<"& "<<s_fq_name <<" = *value;"<<endl;
  os<<"{"<<endl;
  os<<"arma::mat data; data"<<endl;
  for (arma::uword i=0; i<value.n_rows;i++)
  {
    for (arma::uword j=0; j<value.n_cols;j++)
    {
      os<<"<<"<<value(i,j)<<endl;
    }
    os<<"<<arma::endr";
  };
  os<<";"<<endl;
  os<<name<<".reset(new "<<cppParamType(name)<<"(data, \""<<description<<"\")); "<<endl;
  os<<"}"<<endl;
}

