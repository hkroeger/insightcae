#include "matrixparameter.h"
#include "base/rapidxml.h"

namespace insight
{



defineType(MatrixParameter);
addToFactoryTable(Parameter, MatrixParameter);

MatrixParameter::MatrixParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

MatrixParameter::MatrixParameter
(
  const arma::mat& defaultValue,
  const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order
)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  value_(defaultValue)
{}

bool MatrixParameter::isDifferent(const Parameter& p) const
{
  if (const auto* mp=dynamic_cast<const MatrixParameter*>(&p))
  {
    return (*mp)()!=value_;
  }
  else
    return true;
}

void MatrixParameter::set(const arma::mat& nv)
{
  value_=nv;
  triggerValueChanged();
}

const arma::mat& MatrixParameter::operator()() const
{
  return value_;
}

std::string MatrixParameter::latexRepresentation() const
{
  std::ostringstream oss;

  oss<<"\\begin{tabular}{l";
  for (arma::uword j=0; j<value_.n_cols; j++) oss<<'c'<<std::endl;
  oss<<"}\n";

  for (arma::uword i=0;i<value_.n_rows; i++)
  {
    oss<<i<<"&";
    for (arma::uword j=0;j<value_.n_cols; j++)
    {
      oss<<value_(i,j);
      if (j<value_.n_cols-1) oss<<"&";
    }
    oss<<"\\\\"<<std::endl;
  }
  oss<<"\\end{tabular}"<<std::endl;

  return oss.str();
}


std::string MatrixParameter::plainTextRepresentation(int indent) const
{
  std::ostringstream oss;

  for (arma::uword i=0;i<value_.n_rows; i++)
  {
    oss<<std::string(size_t(indent), ' ')<<i<<": ";
    for (arma::uword j=0;j<value_.n_cols; j++)
    {
      oss<<value_(i,j);
      if (j<value_.n_cols-1) oss<<", ";
    }
    oss<<"\n"<<std::endl;
  }

  return oss.str();
}

rapidxml::xml_node< char >* MatrixParameter::appendToNode
(
    const std::string& name,
    rapidxml::xml_document< char >& doc,
    rapidxml::xml_node< char >& node,
    boost::filesystem::path inputfilepath
 ) const
{
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);

  writeMatToXMLNode(value_, doc, *child);

  return child;
}

void MatrixParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_node< char >& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    std::string value_str=child->value();
    std::istringstream iss(value_str);
    value_.load(iss, arma::raw_ascii);
    triggerValueChanged();
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation()
           )
        );
  }
}

std::unique_ptr<Parameter> MatrixParameter::clone(bool init) const
{
    auto p=std::make_unique<MatrixParameter>(
        value_,
        description().simpleLatex());
    if (init) p->initialize();
    return p;
}



void MatrixParameter::copyFrom(const Parameter& p)
{
  operator=(dynamic_cast<const MatrixParameter&>(p));

}

void MatrixParameter::operator=(const MatrixParameter& op)
{
  value_ = op.value_;

  Parameter::copyFrom(op);
}


int MatrixParameter::nChildren() const
{
  return 0;
}




}
