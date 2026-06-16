#include "subsetparameter.h"
#include "base/parameters.h"
#include "base/tools.h"
#include "base/cppextensions.h"
#include "base/translations.h"
#include "base/rapidxml.h"
#include <algorithm>

namespace insight
{







defineType(ParameterSet);
addParameterFactories(ParameterSet);





ParameterSet::ParameterSet()
    : Parameter(std::string(), false, false, false, 0)
{}




ParameterSet::ParameterSet(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{}




ParameterSet::ParameterSet(
    Entries &&defaultValue,
    const std::string &description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
    for (auto& p: defaultValue)
    {
        insertUninitialized(
            p.first,
            std::move(p.second) );
    }
}




ParameterSet::ParameterSet(
    const EntryReferences &defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
    for (auto& p: defaultValue)
    {
        insertUninitialized(
            p.first,
            p.second->cloneAsUninitialized<Parameter>() );
    }
}


Parameter& ParameterSet::insertUninitialized(const std::string &name, std::unique_ptr<Parameter>&& p)
{

    auto ie = value_.find(name);
    if (ie!=value_.end())
    {
        valueChangedConnections_.erase(ie->second.get());
        childValueChangedConnections_.erase(ie->second.get());
        value_.erase(ie);
    }

    auto ins = value_.insert({name, std::move(p)});

    ins.first->second->setParent(this);

    valueChangedConnections_.insert(ins.first->second.get(),
                                    std::make_shared<boost::signals2::scoped_connection>(
                                        ins.first->second->valueChanged.connect(childValueChanged)));
    childValueChangedConnections_.insert(ins.first->second.get(),
                                         std::make_shared<boost::signals2::scoped_connection>(
                                             ins.first->second->childValueChanged.connect( childValueChanged )));

    return *ins.first->second;
}




std::unique_ptr<ParameterSet> ParameterSet::finalize(std::unique_ptr<ParameterSet>&& ps)
{
    ps->initializeHierarchy();
    return std::move(ps);
}



std::unique_ptr<ParameterSet> ParameterSet::create()
{
    std::unique_ptr<ParameterSet> p(new ParameterSet());
    p->initializeHierarchy();
    return p;
}

std::unique_ptr<ParameterSet> ParameterSet::create_uninitialized(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
{
    auto p=std::unique_ptr<ParameterSet>(new ParameterSet(
        description,
        isHidden, isExpert, isNecessary, order ));
    return p;
}

std::unique_ptr<ParameterSet> ParameterSet::create(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
{
    auto p=create_uninitialized(
        description,
        isHidden, isExpert, isNecessary, order );
    p->initializeHierarchy();
    return p;
}

std::unique_ptr<ParameterSet> ParameterSet::create(
    ParameterSet::Entries &&defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
{
    auto p=std::unique_ptr<ParameterSet>(new ParameterSet(
        std::move(defaultValue), description,
        isHidden, isExpert, isNecessary, order ));
    p->initializeHierarchy();
    return p;
}

std::unique_ptr<ParameterSet> ParameterSet::create_uninitialized(
    const ParameterSet::EntryReferences &defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
{
    auto p=std::unique_ptr<ParameterSet>(new ParameterSet(
        defaultValue, description,
        isHidden, isExpert, isNecessary, order ));
    return p;
}

std::unique_ptr<ParameterSet> ParameterSet::create(
    const ParameterSet::EntryReferences &defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
{
    auto p=create_uninitialized(
        defaultValue, description,
        isHidden, isExpert, isNecessary, order );
    p->initializeHierarchy();
    return p;
}

std::unique_ptr<ParameterSet> ParameterSet::create(
    const rapidxml::xml_node<> & node )
{
    auto p=std::unique_ptr<ParameterSet>(
        new ParameterSet( node ));
    // no initialization of hierarchy
    // - hierarchies constructed this way are only intended for use in
    //   documentation (accompanying ResultSets) -
    return p;
}

ParameterSet::EntryReferences ParameterSet::entries() const
{
    EntryReferences result;
    for (auto& p: value_)
    {
        result.insert({p.first, p.second});
    }
    return result;
}




ParameterSet::Entries ParameterSet::copyEntries() const
{
    Entries result;
    for (auto &p: value_)
    {
        result.insert(
            {
             p.first,
             p.second->cloneAsUninitialized<Parameter>()
            });
    }
    return result;
}




bool ParameterSet::isDifferent(const Parameter& p) const
{
  if (const auto *op = dynamic_cast<const ParameterSet*>(&p))
  {
      if (op->value_.size()!=value_.size())
          return true;

      auto thisi=value_.begin(), opi=op->value_.begin();
      while ( (thisi!=value_.end()) && (opi!=op->value_.end()) )
      {
          if ( thisi->first != opi->first )
              return true;

          if ( thisi->second->isDifferent( *(opi->second) ) )
              return true;

          ++thisi; ++opi;
      }

      return false;
  }
  else
    return true;
}




Parameter& ParameterSet::insert(const std::string &name, std::unique_ptr<Parameter>&& p)
{
  auto&ins = insertUninitialized(name, std::move(p));

  ins.initializeHierarchy();
  triggerValueChanged();

  return ins;
}



void ParameterSet::remove(const std::string &name)
{
    auto ie = value_.find(name);
    if (ie!=value_.end())
    {
        valueChangedConnections_.erase(ie->second.get());
        childValueChangedConnections_.erase(ie->second.get());
        value_.erase(ie);
    }
}




std::string ParameterSet::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
  std::string result="";
  if (value_.size()>0)
  {
    result=
        "\\begin{enumerate}\n";
    for(auto i=value_.begin(); i!=value_.end(); i++)
    {
        if (!fsi.elementFilter.matches(*i->second))
        {
          auto ldesc=i->second->description().toLaTeX();

          result+="\\item ";
          if (!ldesc.empty())
              result+=ldesc+"\\\\";
          result+=
              "\n"
              "\\textbf{"+SimpleLatex(i->first).toLaTeX()+"} = "
              +i->second->latexRepresentation(name, documentHierarchyLevel, fsi)
              +"\n";
        }
    }
    result+="\\end{enumerate}\n";
  }
  return result;
}




std::string ParameterSet::plainTextRepresentation(int indent) const
{
  std::string result="\n";
  if (value_.size()>0)
  {
    for(auto i=value_.begin(); i!=value_.end(); i++)
    {
          result+=std::string(indent, ' ')
                  + (i->first)
                  + " = "
                  + i->second->plainTextRepresentation(indent)
                  + '\n';
    }
  }
  return result;
}




bool ParameterSet::isPacked() const
{
  bool is_packed=false;
  for(auto &p: value_)
  {
    is_packed |= p.second->isPacked();
  }
  return is_packed;
}




void ParameterSet::pack()
{
  for(auto &p: value_)
  {
    p.second->pack();
  }
}




void ParameterSet::unpack(const boost::filesystem::path& basePath)
{
  for(auto &p: value_)
  {
    p.second->unpack(basePath);
  }
}




void ParameterSet::clearPackedData()
{
  for(auto &p: value_)
  {
    p.second->clearPackedData();
  }
}




rapidxml::xml_node<>*
ParameterSet::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps ) const
{
  insight::CurrentExceptionContext ex(
        insight::VerbosityLevel::Loops,
        "appending subset %s to node %s", name.c_str(), node.name());

  auto child = Parameter::appendToNode(name, doc, node, outProps);

  for( auto i=value_.begin(); i!= value_.end(); i++)
  {
      if (!outProps.filter.matches(*i->second))
      {
        i->second->appendToNode(i->first, doc, *child, outProps);
      }
  }

  return child;
}




const rapidxml::xml_node<>*
ParameterSet::readFromNode(
    const std::string& name,
    const rapidxml::xml_node<>& node )
{
  using namespace rapidxml;

  auto* child = Parameter::readFromNode(name, node);

  if (child)
  {
    for( auto i=value_.begin(); i!= value_.end(); i++)
    {
          i->second->readFromNode(i->first, *child);
    }
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation(0)
           )
        );
  }

  return child;
}


ParameterSet::ParameterSet(const rapidxml::xml_node<> &node)
    : Parameter(node)
{
    for (auto* e=node.first_node(); e; e=e->next_sibling())
    {
        if (std::string(e->name())!="analysis") // this is no element
        {
            auto name=getMandatoryAttribute(*e, "name");
            insertUninitialized(name, Parameter::createFromNode(*e));
        }
    }
}


int ParameterSet::nChildren() const
{
  return value_.size();
}




std::string ParameterSet::childElementName(int i, bool ) const
{
  auto iter=value_.begin();
  std::advance(iter, i);
  return iter->first;
}

// for any reason, g++ compiler emits warning of
// typecast const Parameter* to int, if this redefinition is removed
std::string ParameterSet::childElementName(
    const Element *p,
    bool redirectArrayElementsToDefault ) const
{
    return Parameter::childElementName(p, redirectArrayElementsToDefault);
}


hierarchicalData::Element& ParameterSet::childElementRef ( int i )
{
  auto iter=value_.begin();
  std::advance(iter, i);
  return *iter->second;
}



const hierarchicalData::Element& ParameterSet::childElement( int i ) const
{
  auto iter=value_.begin();
  std::advance(iter, i);
  return *iter->second;
}




size_t ParameterSet::size() const
{
  return value_.size();
}










bool ParameterSet::contains(const std::string &name) const
{
  return value_.count(name)>0;
}




std::unique_ptr<std::istream>
ParameterSet::getFileStream ( const std::string& name )
{
  return this->get<PathParameter> ( name ) .stream();
}




ParameterSet& ParameterSet::setInt ( const std::string& name, int v )
{
  this->get<IntParameter> ( name ).set( v );
  return *this;
}




ParameterSet& ParameterSet::setDouble ( const std::string& name, double v )
{
  this->get<DoubleParameter> ( name ).set( v );
  return *this;
}




ParameterSet& ParameterSet::setBool ( const std::string& name, bool v )
{
  this->get<BoolParameter> ( name ).set( v );
  return *this;
}





ParameterSet& ParameterSet::setString ( const std::string& name, const std::string& v )
{
  this->get<StringParameter> ( name ).set( v );
  return *this;
}




ParameterSet& ParameterSet::setVector ( const std::string& name, const arma::mat& v )
{
  this->get<VectorParameter> ( name ).set( v );
  return *this;
}




ParameterSet& ParameterSet::setMatrix ( const std::string& name, const arma::mat& m )
{
  this->get<MatrixParameter> ( name ).set( m );
  return *this;
}




ParameterSet& ParameterSet::setOriginalFileName ( const std::string& name, const boost::filesystem::path& fp)
{
  this->get<PathParameter> ( name ).setFilePath(fp);
  return *this;
}


const int& ParameterSet::getInt ( const std::string& name ) const
{
  return this->get<IntParameter> ( name ) ();
}




const double& ParameterSet::getDouble ( const std::string& name ) const
{
  return this->get<DoubleParameter> ( name ) ();
}




const bool& ParameterSet::getBool ( const std::string& name ) const
{
  return this->get<BoolParameter> ( name ) ();
}




const std::string& ParameterSet::getString ( const std::string& name ) const
{
  return this->get<StringParameter> ( name ) ();
}




const arma::mat& ParameterSet::getVector ( const std::string& name ) const
{
  return this->get<VectorParameter> ( name ) ();
}




const boost::filesystem::path ParameterSet::getPath ( const std::string& name, const boost::filesystem::path& basePath ) const
{
  return this->get<PathParameter> ( name ) .filePath();
}




ParameterSet& ParameterSet::getSubset(const std::string& name)
{
  if (name==".")
    return *this;
  else
  {
    return this->get<ParameterSet>(name);
  }
}




const ParameterSet& ParameterSet::operator[] ( const std::string& name ) const
{
  return getSubset ( name );
}




const ParameterSet& ParameterSet::getSubset(const std::string& name) const
{
  if (name==".")
    return *this;
  else
  {
    return this->get<ParameterSet>(name);
  }
}




void ParameterSet::replace ( const std::string& key, std::unique_ptr<Parameter> newp )
{
  using namespace boost;
  using namespace boost::algorithm;

  if ( boost::contains ( key, "/" ) )
  {
    std::string prefix = copy_range<std::string> ( *make_split_iterator ( key, first_finder ( "/" ) ) );
    std::string remain = key;
    erase_head ( remain, prefix.size()+1 );
    return
        this->getSubset ( prefix )
        .replace (
            remain,
            std::move(newp) );
  }
  else
  {
    insert(
          key,
          std::move(newp) );
  }
}




std::unique_ptr<hierarchicalData::Element> ParameterSet::cloneUninitialized() const
{
    auto p =ParameterSet::create_uninitialized(
        entries(),
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order()
        );
    return p;
}



void ParameterSet::assignFrom( const Element& rhs )
{
    auto &osp = dynamic_cast<const ParameterSet&>(rhs);

    std::set<std::string> nonMatching;
    for (auto& te: value_) nonMatching.insert(te.first);

    for (auto &p: osp.value_)
    {
        auto i = value_.find(p.first);
        if (i!=value_.end())
        {
            i->second->assignFrom( *p.second );
        }
        else
        {
            insert(
                p.first,
                p.second->cloneAsUninitialized<Parameter>() );
        }
        if (nonMatching.count(p.first))
            nonMatching.erase(p.first);
    }

    for (auto& nm: nonMatching)
    {
        remove(nm);
    }

    Parameter::assignFrom(osp);
}




void ParameterSet::copyMatching(const Element& rhs)
{
    auto &osp = dynamic_cast<const ParameterSet&>(rhs);

    for (auto &p: osp.value_)
    {
        auto i = value_.find(p.first);
        if (i!=value_.end())
        {
            i->second->copyMatching( *p.second );
        }
    }
}






void ParameterSet::extend ( const Element& other )
{
  auto &osp = dynamic_cast<const ParameterSet&>(other);

  for ( auto &i: osp.value_ )
  {
    if (contains(i.first))
    {
          value_.at(i.first)->extend(*i.second);
    }
    else
    {
          insert(
            i.first,
            i.second->cloneAsUninitialized<Parameter>() );
    }
  }
}



bool ParameterSet::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const ParameterSet*>(&op))
    {
        if (size()!=oa->size()) return false;
        auto i=value_.begin();
        auto j=oa->value_.begin();
        while (i!=value_.end())
        {
            if (i->first!=j->first)
                return false;
            if (!i->second->isEqual(*j->second))
                return false;

            ++i; ++j;
        }
        return true;
    }
    else
        return false;
}






void ParameterSet::clear()
{
    assignFrom(*ParameterSet::create());
}




std::unique_ptr<Parameter> ParameterSet::intersection(const Parameter &other) const
{
  if (auto *osp = dynamic_cast<const ParameterSet*>(&other))
  {
      auto np = ParameterSet::create(
          description().simpleLatex(),
          isHidden(), isExpert(), isNecessary(), order());

      for (auto &p: value_)
      {
        if (osp->value_.count(p.first))
        {
              if (auto pc = p.second->intersection(*osp->value_.find(p.first)->second))
              {
                  np->insert(p.first, std::move(pc));
              }
        }
      }

      return np;
  }

  return nullptr;
}


std::ostream& operator<<(std::ostream& os, const ParameterSet& ps)
{
    CurrentExceptionContext ex(
        2, "writing plain text representation of parameter set to output stream (via << operator)");
    os << ps.plainTextRepresentation(0);
    return os;
}



}
