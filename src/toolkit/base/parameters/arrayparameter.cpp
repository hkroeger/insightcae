#include "arrayparameter.h"
#include "base/translations.h"


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


bool ArrayParameter::isDifferent(const Parameter& p) const
{
  if (const auto* ap = dynamic_cast<const ArrayParameter*>(&p))
  {
    if (ap->size()!=size())
      return true;

    for (int i=0; i<size(); ++i)
    {
      if ( element(i).isDifferent( ap->element(i) ) )
        return true;
    }

    return false;
  }
  else
    return true;
}



void ArrayParameter::setDefaultValue ( const Parameter& defP )
{
  defaultValue_.reset ( defP.clone() );
}


const Parameter& ArrayParameter::defaultValue() const
{
  return *defaultValue_;
}


int ArrayParameter::defaultSize() const
{
  return defaultSize_;
}


void ArrayParameter::eraseValue ( int i )
{
  value_.erase ( value_.begin()+i );
  valueChanged();
}


void ArrayParameter::appendValue ( const Parameter& np )
{
  value_.push_back ( ParameterPtr( np.clone() ) );
  valueChanged();
}


void ArrayParameter::insertValue ( int i, const Parameter& np )
{
  value_.insert( value_.begin()+i, ParameterPtr( np.clone() ) );
  valueChanged();
}


void ArrayParameter::appendEmpty()
{
  value_.push_back ( ParameterPtr( defaultValue_->clone() ) );
  valueChanged();
}


Parameter& ArrayParameter::operator[] ( int i )
{
  return elementRef(i);
}


const Parameter& ArrayParameter::operator[] ( int i ) const
{
  return element(i);
}

const Parameter& ArrayParameter::element(int i) const
{
  return *(value_[i]);
}




int ArrayParameter::size() const
{
    return value_.size();
}


void ArrayParameter::clear()
{
    value_.clear();
    valueChanged();
}

std::string ArrayParameter::latexRepresentation() const
{
  std::ostringstream os;
  if (size()>0)
  {
    os<<"\\begin{enumerate}\n";

    for(value_type::const_iterator i=value_.begin(); i!=value_.end(); ++i)
    {
      os << "\\item item " << (i-value_.begin()) << " :\\\\\n" << (*i)->latexRepresentation();
    }
    os << "\\end{enumerate}\n";
  }
  else
  {
    os << "(empty)\n";
  }
  return os.str();
}




std::string ArrayParameter::plainTextRepresentation(int indent) const
{
  std::ostringstream os;
  if (size()>0)
  {
    os << "\n";
    for(value_type::const_iterator i=value_.begin(); i!=value_.end(); i++)
    {
      os << std::string(indent+1, ' ')  << "item " << (i-value_.begin()) << " :\n"
         << std::string(indent+1, ' ')  << (*i)->plainTextRepresentation(indent+1);
    }
  }
  else
  {
    os << "(empty)\n";
  }
  return os.str();
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




rapidxml::xml_node<>* ArrayParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
  insight::CurrentExceptionContext ex("appending array "+name+" to node "+node.name());
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  defaultValue_->appendToNode("default", doc, *child, inputfilepath);
  for (int i=0; i<size(); i++)
  {
    value_[i]->appendToNode(boost::lexical_cast<std::string>(i), doc, *child, inputfilepath);
  }
  return child;
}




void ArrayParameter::readFromNode(const std::string& name, rapidxml::xml_node<>& node,
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
        defaultValue_->readFromNode( name, *child, inputfilepath );
      }
      else
      {
        int i=boost::lexical_cast<int>(name);
        ParameterPtr p(defaultValue_->clone());
        p->readFromNode( name, *child, inputfilepath );

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

    valueChanged();
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
                _("No xml node found with type '%s' and name '%s', default value '%s' is used.")
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
    valueChanged();
  }
  else
    throw insight::Exception(_("Tried to set a %s from a different type (%s)!"), type().c_str(), p.type().c_str());
}





}
