#include "matrixparameterparser.h"

using namespace std;

MatrixParameterParser::Data::Data(arma::uword r, arma::uword c, const std::string& d)
: ParserDataBase(d), value(arma::zeros(r,c))
{}

MatrixParameterParser::Data::Data(const std::vector<std::vector<double> >& mat, const std::string& d)
: ParserDataBase(d)
{
  size_t r=mat.size();
  if (r<1)
    throw PDLException("Empty matrix is not allowed!");
  size_t c=mat[0].size();
  if (c<1)
    throw PDLException("Matrix with zero columns is not allowed!");

  value=arma::zeros(r,c);

  for (size_t i=0; i<r; i++)
  {
    if (mat[i].size()!=c)
      throw PDLException("Invalid row in matrix definition!");

    for (size_t j=0; j<c; j++)
    {
      value(i,j)=mat[i][j];
    }
  }
}

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
  std::ostringstream os;
  os<<"arma::mat(boost::assign::list_of";
  for (size_t i=0; i<value.n_rows; i++)
  {
    for (size_t j=0; j<value.n_cols; j++)
    {
      os<<"("<<value(i,j)<<")";
    }
  }
  os<<".convert_to_container<std::vector<double> >().data(), "<<value.n_rows<<", "<<value.n_cols<<")";
  return os.str();
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

