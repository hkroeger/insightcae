#include "subsetparameter.h"
#include "base/parameters.h"
#include "base/tools.h"
#include "base/cppextensions.h"
#include "base/translations.h"

namespace insight
{



ParameterNotFoundException::ParameterNotFoundException(
    const std::string &msg)
    : Exception(msg)
{}





defineType(SubsetParameter);
addToFactoryTable(Parameter, SubsetParameter);




SubsetParameter::SubsetParameter()
{
}




SubsetParameter::SubsetParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}




SubsetParameter::SubsetParameter(
    const EntryCopies &defaultValue,
    const std::string &description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
    for (auto& p: defaultValue)
    {
        insert(
            p.first,
            std::unique_ptr<Parameter>(
                p.second->clone() ) );
    }
}




SubsetParameter::SubsetParameter(
    const EntryReferences &defaultValue,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
    for (auto& p: defaultValue)
    {
        insert(
            p.first,
            std::unique_ptr<Parameter>(
                p.second->clone() ) );
    }
}




SubsetParameter::EntryReferences SubsetParameter::entries() const
{
    EntryReferences result;
    for (auto p: value_)
    {
        result.insert({p->first, p->second});
    }
    return result;
}




SubsetParameter::EntryCopies SubsetParameter::copyEntries() const
{
    EntryCopies result;
    for (auto p: value_)
    {
        result.insert(
            {
             p->first,
             std::shared_ptr<Parameter>(
                 p->second->clone() )
            });
    }
    return result;
}




bool SubsetParameter::isDifferent(const Parameter& p) const
{
  if (const auto *op = dynamic_cast<const SubsetParameter*>(&p))
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




void SubsetParameter::insert(const std::string &name, std::unique_ptr<Parameter> p)
{

  auto ie = value_.find(name);
  if (ie!=value_.end())
  {
    valueChangedConnections_.erase(ie->second);
    childValueChangedConnections_.erase(ie->second);
    value_.erase(ie);
  }

  auto ins = value_.insert(name, std::auto_ptr<Parameter>(p.release()) );

  ins.first->second->setParent(this);

  valueChangedConnections_[ins.first->second]=
      std::make_shared<boost::signals2::scoped_connection>(
      ins.first->second->valueChanged.connect(childValueChanged));
  childValueChangedConnections_[ins.first->second]=
      std::make_shared<boost::signals2::scoped_connection>(
      ins.first->second->childValueChanged.connect( childValueChanged ));

  triggerValueChanged();
}




std::string SubsetParameter::latexRepresentation() const
{
  std::string result="";
  if (value_.size()>0)
  {
    result=
        "\\begin{enumerate}\n";
    for(auto i=value_.begin(); i!=value_.end(); i++)
    {
          auto ldesc=i->second->description().toLaTeX();

          result+="\\item ";
          if (!ldesc.empty())
              result+=ldesc+"\\\\";
          result+=
              "\n"
              "\\textbf{"+SimpleLatex(i->first).toLaTeX()+"} = "
              +i->second->latexRepresentation()
              +"\n";
    }
    result+="\\end{enumerate}\n";
  }
  return result;
}




std::string SubsetParameter::plainTextRepresentation(int indent) const
{
  std::string result="\n";
  if (value_.size()>0)
  {
    for(auto i=value_.begin(); i!=value_.end(); i++)
    {
          result+=std::string(indent, ' ') + (i->first) + " = " + i->second->plainTextRepresentation(indent) + '\n';
    }
  }
  return result;
}




bool SubsetParameter::isPacked() const
{
  bool is_packed=false;
  for(auto p: value_)
  {
    is_packed |= p.second->isPacked();
  }
  return is_packed;
}




void SubsetParameter::pack()
{
  for(auto p: value_)
  {
    p.second->pack();
  }
}




void SubsetParameter::unpack(const boost::filesystem::path& basePath)
{
  for(auto p: value_)
  {
    p.second->unpack(basePath);
  }
}




void SubsetParameter::clearPackedData()
{
  for(auto p: value_)
  {
    p.second->clearPackedData();
  }
}




rapidxml::xml_node<>* SubsetParameter::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath ) const
{
  insight::CurrentExceptionContext ex(2, "appending subset "+name+" to node "+node.name());

  using namespace rapidxml;
  xml_node<>*  child = Parameter::appendToNode(name, doc, node, inputfilepath);

  for( auto i=value_.begin(); i!= value_.end(); i++)
  {
    i->second->appendToNode(i->first, doc, *child, inputfilepath);
  }

  return child;
}




void SubsetParameter::readFromNode(
    const std::string& name,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;

  xml_node<>* child = findNode(node, name, type());

  if (child)
  {
    for( auto i=value_.begin(); i!= value_.end(); i++)
    {
          i->second->readFromNode(i->first, *child, inputfilepath);
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




int SubsetParameter::nChildren() const
{
  return value_.size();
}




std::string SubsetParameter::childParameterName(int i) const
{
  auto iter=value_.begin();
  std::advance(iter, i);
  return iter->first;
}




Parameter& SubsetParameter::childParameterRef ( int i )
{
  auto iter=value_.begin();
  std::advance(iter, i);
  return *iter->second;
}



const Parameter& SubsetParameter::childParameter( int i ) const
{
  auto iter=value_.begin();
  std::advance(iter, i);
  return *iter->second;
}




size_t SubsetParameter::size() const
{
  return value_.size();
}




std::string splitOffFirstParameter(std::string& path, int& nRemaining)
{
  using namespace boost;
  using namespace boost::algorithm;

  if ( boost::contains ( path, "/" ) )
  {
    std::string prefix = copy_range<std::string> ( *make_split_iterator ( path, first_finder ( "/" ) ) );

    std::string remain = path;
    erase_head ( remain, prefix.size()+1 );

    path=remain;
    nRemaining = std::count(path.begin(), path.end(), '/')+1;
    return prefix;
  }
  else
  {
    std::string prefix=path;
    path="";
    nRemaining=0;
    return prefix;
  }
}




bool SubsetParameter::hasParameter(std::string path) const
{

  std::function<bool(const Parameter&, std::string)> checkChildren;

  checkChildren = [&checkChildren](const Parameter& cp, std::string path)
  {
      int nRemaining=-1;
      std::string parameterName = splitOffFirstParameter(path, nRemaining);
      int i = cp.childParameterIndex(parameterName);
      if (i==-1)
      {
        return false;
      }
      else if (nRemaining == 0)
      {
        return true;
      }
      else
      {
        return checkChildren(cp.childParameter(i), path);
      }
  };

  return checkChildren(*this, path);
}




Parameter& SubsetParameter::getParameter(std::string path)
{

  std::function<Parameter&(Parameter&, std::string)> getChild;

  getChild = [&getChild,this](Parameter& cp, std::string path) -> Parameter&
  {
      int nRemaining=-1;
      std::string parameterName = splitOffFirstParameter(path, nRemaining);
      if (parameterName=="..")
      {
          if (auto p=cp.parent())
          {
              return getChild(*p, path);
          }
          else
              throw insight::ParameterNotFoundException(
                  str(boost::format(
                     _("relative path given (%s) but no parent parameter container set!"))
                   % path ) );
      }
      int i = cp.childParameterIndex(parameterName);
      if (i==-1)
      {
          throw insight::ParameterNotFoundException("There is no parameter with name "+parameterName);
      }
      else if (nRemaining == 0)
      {
          return cp.childParameterRef(i);
      }
      else
      {
          return getChild(cp.childParameterRef(i), path);
      }
  };

  return getChild(*this, path);
}




bool SubsetParameter::contains(const std::string &name) const
{
  return value_.count(name)>0;
}




std::istream& SubsetParameter::getFileStream ( const std::string& name )
{
  return this->get<PathParameter> ( name ) .stream();
}




SubsetParameter& SubsetParameter::setInt ( const std::string& name, int v )
{
  this->get<IntParameter> ( name ).set( v );
  return *this;
}




SubsetParameter& SubsetParameter::setDouble ( const std::string& name, double v )
{
  this->get<DoubleParameter> ( name ).set( v );
  return *this;
}




SubsetParameter& SubsetParameter::setBool ( const std::string& name, bool v )
{
  this->get<BoolParameter> ( name ).set( v );
  return *this;
}





SubsetParameter& SubsetParameter::setString ( const std::string& name, const std::string& v )
{
  this->get<StringParameter> ( name ).set( v );
  return *this;
}




SubsetParameter& SubsetParameter::setVector ( const std::string& name, const arma::mat& v )
{
  this->get<VectorParameter> ( name ).set( v );
  return *this;
}




SubsetParameter& SubsetParameter::setMatrix ( const std::string& name, const arma::mat& m )
{
  this->get<MatrixParameter> ( name ).set( m );
  return *this;
}




SubsetParameter& SubsetParameter::setOriginalFileName ( const std::string& name, const boost::filesystem::path& fp)
{
  this->get<PathParameter> ( name ).setOriginalFilePath(fp);
  return *this;
}


const int& SubsetParameter::getInt ( const std::string& name ) const
{
  return this->get<IntParameter> ( name ) ();
}




const double& SubsetParameter::getDouble ( const std::string& name ) const
{
  return this->get<DoubleParameter> ( name ) ();
}




const bool& SubsetParameter::getBool ( const std::string& name ) const
{
  return this->get<BoolParameter> ( name ) ();
}




const std::string& SubsetParameter::getString ( const std::string& name ) const
{
  return this->get<StringParameter> ( name ) ();
}




const arma::mat& SubsetParameter::getVector ( const std::string& name ) const
{
  return this->get<VectorParameter> ( name ) ();
}




const boost::filesystem::path SubsetParameter::getPath ( const std::string& name, const boost::filesystem::path& basePath ) const
{
  return this->get<PathParameter> ( name ) .filePath(basePath);
}




SubsetParameter& SubsetParameter::getSubset(const std::string& name)
{
  if (name==".")
    return *this;
  else
  {
    return this->get<SubsetParameter>(name);
  }
}




const SubsetParameter& SubsetParameter::operator[] ( const std::string& name ) const
{
  return getSubset ( name );
}




const SubsetParameter& SubsetParameter::getSubset(const std::string& name) const
{
  if (name==".")
    return *this;
  else
  {
    return this->get<SubsetParameter>(name);
  }
}




void SubsetParameter::replace ( const std::string& key, Parameter* newp )
{
  using namespace boost;
  using namespace boost::algorithm;

  if ( boost::contains ( key, "/" ) )
  {
    std::string prefix = copy_range<std::string> ( *make_split_iterator ( key, first_finder ( "/" ) ) );
    std::string remain = key;
    erase_head ( remain, prefix.size()+1 );
    return this->getSubset ( prefix ).replace ( remain, newp );
  }
  else
  {
    insert(key, std::unique_ptr<Parameter>(newp));
  }
}




Parameter* SubsetParameter::clone() const
{
  return new SubsetParameter(
      entries(),
      description_.simpleLatex(),
      isHidden_, isExpert_, isNecessary_, order_
      );
}




void SubsetParameter::copyFrom(const Parameter& o)
{
  operator=(dynamic_cast<const SubsetParameter&>(o));

}




void SubsetParameter::operator=(const SubsetParameter& osp)
{
  for (auto p: osp.value_)
  {
    auto i = value_.find(p.first);
    if (i!=value_.end())
          i->second->copyFrom( *p.second );
    else
          insert(p.first, std::unique_ptr<Parameter>(p.second->clone()));
  }
  Parameter::copyFrom(osp);
}




void SubsetParameter::extend ( const Parameter& other )
{
  auto &osp = dynamic_cast<const SubsetParameter&>(other);
  for ( auto i: osp.value_ )
  {
    if (contains(i.first))
    {
          value_.at(i.first).extend(*i.second);
    }
    else
    {
          insert( i.first, std::unique_ptr<Parameter>(i.second->clone()) );
    }
  }
}




void SubsetParameter::merge ( const Parameter& other )
{
  if (auto *osp = dynamic_cast<const SubsetParameter*>(&other))
  {
      for ( auto i: osp->value_ )
      {
        if (contains(i.first))
        {
              value_.at(i.first).merge(*i.second);
        }
      }
  }
}




std::unique_ptr<Parameter> SubsetParameter::intersection(const Parameter &other) const
{
  if (auto *osp = dynamic_cast<const SubsetParameter*>(&other))
  {
      auto np = std::make_unique<SubsetParameter>(
          description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);

      for (auto p: value_)
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






}
