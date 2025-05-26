#include "arrayparameter.h"
#include "base/translations.h"
#include "base/tools.h"
#include "base/rapidxml.h"

namespace insight
{





defineType(ArrayParameter);
addToFactoryTable(Parameter, ArrayParameter);




ArrayParameter::ArrayParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  defaultSize_(0)
{}




ArrayParameter::ArrayParameter(const Parameter& defaultValue, int size, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  defaultSize_(size)
{
    setDefaultValue(defaultValue.clone(false));
    for (int i=0; i<defaultSize_; i++)
    {
        appendEmpty(false);
    }
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



void ArrayParameter::setDefaultValue (
    std::unique_ptr<Parameter>&& defP )
{
  defaultValue_=std::move(defP);
}


const Parameter& ArrayParameter::defaultValue() const
{
  return *defaultValue_;
}


int ArrayParameter::defaultSize() const
{
  return defaultSize_;
}

void ArrayParameter::resize(int newSize, bool init)
{
  if (newSize<size())
  {
    for (int k=size()-1; k>=newSize; --k)
    {
      eraseValue(k);
    }
  }
  else if (newSize>size())
  {
    for (int k=size(); k<newSize; ++k)
    {
      appendEmpty(init);
    }
  }

  insight::assertion(
        newSize==size(),
      "internal error: wrongly resized array!");
}


void ArrayParameter::eraseValue ( int i )
{
  insight::assertion(
    i>=0 && i<size(),
      "%d out of range (0...%d)", i, size()-1);

  beforeChildRemoval(i, i);

  auto idx=value_.begin();
  std::advance(idx, i);

  valueChangedConnections_.erase(idx->get());
  childValueChangedConnections_.erase(idx->get());

  // auto item=std::move(*idx); // don't delete yet, this would cause crash in IQParameterSetModel.
  value_.erase ( idx ); // Just remove from array

  childRemovalDone(i, i);

  triggerValueChanged();
}


void ArrayParameter::appendValue ( std::unique_ptr<Parameter>&& np )
{
    int i=value_.size();
  beforeChildInsertion(i, i);

  value_.push_back ( std::move(np) );
  auto& ins=value_.back();

  valueChangedConnections_.insert(ins.get(),
      std::make_shared<boost::signals2::scoped_connection>(
          ins->valueChanged.connect( childValueChanged )));
  childValueChangedConnections_.insert(ins.get(),
      std::make_shared<boost::signals2::scoped_connection>(
          ins->childValueChanged.connect( childValueChanged )));
  newItemAdded(ins.get());
  ins->setParent(this);

  childInsertionDone(i, i);

  triggerValueChanged();
}


void ArrayParameter::insertValue ( int i, std::unique_ptr<Parameter>&& np )
{
  insight::assertion(
      i>=0 && i<size(),
      "%d out of range (0...%d)", i, size()-1);

  beforeChildInsertion(i, i);

  auto ins = value_.insert( value_.begin()+i, std::move(np) );

  valueChangedConnections_.insert(ins->get(),
      std::make_shared<boost::signals2::scoped_connection>(
       (*ins)->valueChanged.connect( childValueChanged )));
  childValueChangedConnections_.insert(ins->get(),
      std::make_shared<boost::signals2::scoped_connection>(
       (*ins)->childValueChanged.connect( childValueChanged )));
  newItemAdded(ins->get());
  (*ins)->setParent(this);

  childInsertionDone(i, i);

  triggerValueChanged();
}


void ArrayParameter::appendEmpty(bool init)
{
    int i=value_.size();
    beforeChildInsertion(i, i);

  value_.push_back ( defaultValue_->clone(false) );
  if (init) initialize();

  auto& ins=value_.back();
  valueChangedConnections_.insert(ins.get(),
      std::make_shared<boost::signals2::scoped_connection>(
        ins->valueChanged.connect( childValueChanged )));
  childValueChangedConnections_.insert(ins.get(),
      std::make_shared<boost::signals2::scoped_connection>(
        ins->childValueChanged.connect( childValueChanged )));
  newItemAdded(ins.get());
  ins->setParent(this);

  childInsertionDone(i, i);

  triggerValueChanged();
}


Parameter& ArrayParameter::operator[] ( int i )
{
  return elementRef(i);
}

Parameter &ArrayParameter::elementRef(int i)
{
    return *(value_[i]);
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

int ArrayParameter::nChildren() const
{
    return value_.size();
}

std::string ArrayParameter::childParameterName(
    int i,
    bool redirectArrayElementsToDefault ) const
{
    if ( redirectArrayElementsToDefault || (i==nChildren()) )
        return "default";
    else if (i<nChildren())
        return str(boost::format("%d")%i);
    else
    {
        throw insight::Exception(
            "index %d out of range (0...%d)", i, nChildren() );
        return std::string();
    }
}


Parameter& ArrayParameter::childParameterRef ( int i )
{
    insight::assertion(i>=0 && i<=nChildren(),
                       "index %d out of range (0...%d)", i, nChildren());
    if (i==nChildren())
        return *defaultValue_;
    else
        return *(value_[i]);
}


const Parameter& ArrayParameter::childParameter( int i ) const
{
    insight::assertion(i>=0 && i<=nChildren(),
                       "index %d out of range (0...%d)", i, nChildren());
    if (i==nChildren())
        return *defaultValue_;
    else
        return *(value_[i]);
}

int ArrayParameter::childParameterIndex(const std::string &name) const
{
    if (name=="default")
    {
        return nChildren();
    }
    else if (name=="first")
    {
        if (size()>0) return 0;
    }
    else if (name=="last")
    {
        if (size()>0) return size()-1;
    }
    try
    {
        int i = boost::lexical_cast<int>( boost::algorithm::trim_copy(name) );
        if (i<0) i=size()+i;
        if (i>=0 && i<nChildren())
        {
            return i;
        }
    }
    catch (const boost::bad_lexical_cast& e)
    {}
    return -1;
}



void ArrayParameter::clear()
{

    valueChangedConnections_.clear();
    childValueChangedConnections_.clear();
    value_.clear();

    triggerValueChanged();
}

std::string ArrayParameter::latexRepresentation() const
{
  std::ostringstream os;
  if (size()>0)
  {
    os<<"\\begin{enumerate}\n";

    for(auto i=value_.begin(); i!=value_.end(); ++i)
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
    for(auto i=value_.begin(); i!=value_.end(); i++)
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
  insight::CurrentExceptionContext ex(3, "appending array "+name+" to node "+node.name());
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  defaultValue_->appendToNode("default", doc, *child, inputfilepath);
  for (int i=0; i<size(); i++)
  {
    value_[i]->appendToNode(
          boost::lexical_cast<std::string>(i),
          doc, *child,
          inputfilepath );
  }
  return child;
}




void ArrayParameter::readFromNode(
    const std::string& name,
    const rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  auto* child = findNode(node, name, type());

  if (child)
  {
    int imax=-1;
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
        imax=std::max(imax,i);
        if (i>size()-1) resize(i+1, false);
        value_[i]->readFromNode( name, *child, inputfilepath );
      }
    }
    if (imax<0)
      clear();
    else
      resize(imax+1, false);

    initialize();
    triggerValueChanged();
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




std::unique_ptr<Parameter> ArrayParameter::clone (bool init) const
{
  auto np=std::make_unique<ArrayParameter>(
        *defaultValue_,
        0,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order());

  for (int i=0; i<size(); i++)
  {
    np->appendValue( value_[i]->clone(false) );
  }

  if (init) np->initialize();

  return np;
}




void ArrayParameter::copyFrom(const Parameter& p)
{
  operator=(dynamic_cast<const ArrayParameter&>(p));
}




void ArrayParameter::operator=(const ArrayParameter& op)
{
  (*defaultValue_).copyFrom(*op.defaultValue_);
  defaultSize_ = op.defaultSize_;

  resize(op.size(), true);

  for (int i=0; i<op.size(); ++i)
  {
    (*value_[i]).copyFrom( *op.value_[i] );
  }

  Parameter::copyFrom(op);
}




void ArrayParameter::extend(const Parameter &other)
{
  auto &op = dynamic_cast<const ArrayParameter&>(other);
  defaultValue_->extend(*op.defaultValue_);
  for (auto&e : value_)
  {
    e->extend(*op.defaultValue_);
  }
  triggerValueChanged();
}

void ArrayParameter::merge(const Parameter &other)
{
  auto &op = dynamic_cast<const ArrayParameter&>(other);

  defaultValue_->merge(*op.defaultValue_);

  int i;
  for (i=0; i<std::min(size(), op.size()); ++i)
  {
    value_[i]->merge(*op.value_[i]);
  }
  for (; i<op.size(); ++i) // if other is larger
  {
    appendEmpty(true);
    insight::assertion( value_.size()-1==i, "unexpected" );
    value_.back()->merge(*op.value_[i]);
  }
  triggerValueChanged();
}





}
