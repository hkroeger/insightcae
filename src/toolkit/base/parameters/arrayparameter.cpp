#include "arrayparameter.h"



namespace insight
{





defineType(ArrayParameter);
addToFactoryTable(Parameter, ArrayParameter);

ArrayParameter::ArrayParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  defaultSize_(0)
{
}

ArrayParameter::ArrayParameter(const Parameter& defaultValue, int size, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  defaultValue_(defaultValue.clone()),
  defaultSize_(size)
{
  for (int i=0; i<size; i++) appendEmpty();
}


std::string ArrayParameter::latexRepresentation() const
{
  return std::string();
}

std::string ArrayParameter::plainTextRepresentation(int) const
{
  return std::string();
}

bool ArrayParameter::isPacked() const
{
  bool is_packed=false;
  for (const auto& p: value_)
  {
    is_packed |= p->isPacked();
  }
  return is_packed;
}

void ArrayParameter::pack()
{
  for (auto& p: value_)
  {
    p->pack();
  }
}

void ArrayParameter::unpack(const boost::filesystem::path& basePath)
{
  for (auto& p: value_)
  {
    p->unpack(basePath);
  }
}

void ArrayParameter::clearPackedData()
{
  for (auto& p: value_)
  {
    p->clearPackedData();
  }
}


rapidxml::xml_node<>* ArrayParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  defaultValue_->appendToNode("default", doc, *child, inputfilepath);
  for (int i=0; i<size(); i++)
  {
    value_[i]->appendToNode(boost::lexical_cast<std::string>(i), doc, *child, inputfilepath);
  }
  return child;
}

void ArrayParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());

  std::vector<std::pair<double, ParameterPtr> > readvalues;

  if (child)
  {
    value_.clear();
    for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
    {
      std::string name(e->first_attribute("name")->value());
      if (name=="default")
      {
        defaultValue_->readFromNode( name, doc, *child, inputfilepath );
      }
      else
      {
        int i=boost::lexical_cast<int>(name);
        ParameterPtr p(defaultValue_->clone());
        p->readFromNode( name, doc, *child, inputfilepath );

        readvalues.push_back( decltype(readvalues)::value_type(i, p) );
      }
    }

    sort(readvalues.begin(), readvalues.end(),
         [](const decltype(readvalues)::value_type& v1, const decltype(readvalues)::value_type& v2)
            {
                return v1.first < v2.first;
            }
    );

    for (const auto& v: readvalues)
    {
        value_.push_back(v.second);
    }
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



Parameter* ArrayParameter::clone () const
{
  ArrayParameter* np=new ArrayParameter(*defaultValue_, 0, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
  for (int i=0; i<size(); i++)
  {
    np->appendValue( *(value_[i]) );
  }
  return np;
}


void ArrayParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const ArrayParameter*>(&p))
  {
    Parameter::reset(p);
    defaultValue_.reset( op->defaultValue_->clone() );
    defaultSize_ = op->defaultSize_;
    value_.clear();
    for (const auto& v: op->value_)
      value_.push_back( ParameterPtr(v->clone()) );
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}





}
