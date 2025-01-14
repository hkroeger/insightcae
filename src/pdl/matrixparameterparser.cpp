#include "matrixparameterparser.h"

using namespace std;

defineType(MatrixParameterParser);
addToStaticFunctionTable(ParameterGenerator, MatrixParameterParser, insertrule);


MatrixParameterParser::MatrixParameterParser(
    arma::uword r, arma::uword c, const std::string& d)
: ParameterGenerator(d), value(arma::zeros(r,c))
{}

MatrixParameterParser::MatrixParameterParser(
    const std::vector<std::vector<double> >& mat, const std::string& d)
: ParameterGenerator(d)
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


void MatrixParameterParser::cppAddRequiredInclude(std::set<std::string>& headers) const
{
    headers.insert("<armadillo>");
    headers.insert("\"base/parameters/matrixparameter.h\"");
}


std::string MatrixParameterParser::cppInsightType() const
{
  return "insight::MatrixParameter";
}


std::string MatrixParameterParser::cppStaticType() const
{
    return "arma::mat";
}


std::string MatrixParameterParser::cppDefaultValueExpression() const
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


void MatrixParameterParser::cppWriteCreateStatement(
    std::ostream& os,
    const std::string& psvarname ) const
{

  os<<"std::unique_ptr< "<<cppInsightType()<<" > "<<psvarname<<";"<<endl;
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
  os<<psvarname<<".reset(new "<<cppInsightType()<<"(data, "<<cppInsightTypeConstructorParameters()<<"));\n";
  os<<"}"<<endl;
}

