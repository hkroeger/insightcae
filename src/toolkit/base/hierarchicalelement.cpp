#include "hierarchicalelement.h"

#include "base/rapidxml.h"
#include "base/tools.h"
#include "base/translations.h"
#include "boost/filesystem/operations.hpp"

namespace insight {



namespace hierarchicalData {



std::string
elementPath::join(const std::string& p1, const std::string& p2)
{
    return
        p1
        + (
            (!p1.empty()) && (!p2.empty())
                ? "/" : ""
            ) +
        p2;
}




std::string
elementPath::join(const std::vector<std::string>& ps)
{
    std::string result;
    for (auto& p: ps)
    {
        result=join(result, p);
    }
    return result;
}


Ordering::Ordering ( double ordering_base, double ordering_step_fraction )
    : ordering_ ( ordering_base ),
      step_ ( ordering_base*ordering_step_fraction )
{}

double Ordering::next()
{
    ordering_+=step_;
    return ordering_;
}




LaTeXRepresentableValue::LaTeXRepresentableValue()
    : displayFullPage_(false)
{}

LaTeXRepresentableValue::~LaTeXRepresentableValue()
{}

void LaTeXRepresentableValue::setDisplayFullPage(bool displayFullPage)
{
    displayFullPage_=displayFullPage;
}

bool LaTeXRepresentableValue::displayFullPage() const
{
    return displayFullPage_;
}

void LaTeXRepresentableValue::insertLatexHeaderCode(
    std::set<std::string> &headerCode ) const
{}



defineType(Element);


Element::iterator::iterator()
    : p_(nullptr),
    iChild_(-1)
{}


Element::iterator::iterator(Element& p, int i)
  : p_(&p),
    iChild_(i)
{}

Element::iterator::iterator(const iterator& i)
  : p_(i.p_),
    iChild_(i.iChild_)
{}

Element::iterator::~iterator()
{}

Element::iterator& Element::iterator::operator=(const iterator&o)
{
  auto other=o;
  std::swap(*this, other);
  return *this;
}

bool Element::iterator::operator==(const iterator& o) const
{
  return (o.p_==p_) && (o.iChild_==iChild_);
}

bool Element::iterator::operator!=(const iterator& o) const
{
  return !operator==(o);
}


Element::iterator& Element::iterator::operator++()
{
  if (p_)
  {
        iChild_++;
  }
  return *this;
}

Element::iterator::reference Element::iterator::operator*() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(),
      "invalid iterator (out of range 0..%d)",
      p_->nChildren()-1 );

  return p_->childElementRef(iChild_);
}

Element::iterator::pointer Element::iterator::operator->() const
{
  return &operator*();
}

Element::iterator::pointer Element::iterator::get_pointer() const
{
  return &operator*();
}

std::string Element::iterator::name() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(),
      "invalid iterator (out of range 0..%d)",
      p_->nChildren()-1 );

  return p_->childElementName(iChild_);
}




Element::const_iterator::const_iterator()
  : p_(nullptr),
    iChild_(-1)
{}

Element::const_iterator::const_iterator(const Element& p, int i)
  : p_(&p),
    iChild_(i)
{}

Element::const_iterator::const_iterator(const iterator& i)
  : p_(i.p_),
    iChild_(i.iChild_)
{}

Element::const_iterator::const_iterator(const const_iterator& i)
  : p_(i.p_),
    iChild_(i.iChild_)
{}

Element::const_iterator::~const_iterator()
{}

Element::const_iterator& Element::const_iterator::operator=(const const_iterator& o)
{
  auto other=o;
  std::swap(*this, other);
  return *this;
}

bool Element::const_iterator::operator==(const const_iterator& o) const
{
  return (o.p_==p_) && (o.iChild_==iChild_);
}

bool Element::const_iterator::operator!=(const const_iterator& o) const
{
  return !operator==(o);
}

Element::const_iterator& Element::const_iterator::operator++()
{
  if (p_)
  {
        iChild_++;
  }
  return *this;
}

Element::const_iterator::reference Element::const_iterator::operator*() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(),
      "invalid iterator (out of range 0..%d)", p_->nChildren()-1 );

  return p_->childElement(iChild_);
}

Element::const_iterator::pointer Element::const_iterator::operator->() const
{
  return &operator*();
}

Element::const_iterator::pointer Element::const_iterator::get_pointer() const
{
  return &operator*();
}

std::string Element::const_iterator::name() const
{
  insight::assertion(
      p_!=nullptr, "invalid iterator (no reference)" );
  insight::assertion(
      iChild_>=0 && iChild_<p_->nChildren(), "invalid iterator (out of range 0..%d)", p_->nChildren()-1 );

  return p_->childElementName(iChild_);
}


void Element::triggerValueChanged()
{
    if (!valueChangeSignalBlocked_) valueChanged();
}

void Element::triggerChildValueChanged()
{
    if (!valueChangeSignalBlocked_) childValueChanged();
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


bool Element::hasPath( std::string path ) const
{

    std::function<bool(const Element&, std::string)> checkChildren;

    checkChildren = [&checkChildren](const Element& cp, std::string path)
    {
        int nRemaining=-1;
        std::string parameterName = splitOffFirstParameter(path, nRemaining);
        int i = cp.childElementIndex(parameterName);
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
            return checkChildren(cp.childElement(i), path);
        }
    };

    return checkChildren(*this, path);
}




insight::hierarchicalData::Element& Element::getByPath( std::string path )
{

    std::function<Element&(Element&, std::string)> getChild;

    getChild = [&getChild,this](Element& cp, std::string path) -> Element&
    {
        int nRemaining=-1;
        std::string parameterName = splitOffFirstParameter(path, nRemaining);
        if (parameterName=="..")
        {
            if (cp.hasParent())
            {
                return getChild(cp.parent(), path);
            }
            else
                throw insight::ElementNotFoundException(
                    str(boost::format(
                            _("relative path given (%s) but no parent parameter container set!"))
                        % path ) );
        }
        int i = cp.childElementIndex(parameterName);
        if (i==-1)
        {
            throw insight::ElementNotFoundException("There is no parameter with name "+parameterName);
        }
        else if (nRemaining == 0)
        {
            return cp.childElementRef(i);
        }
        else
        {
            return getChild(cp.childElementRef(i), path);
        }
    };

    return getChild(*this, path);
}




void Element::setParent(Element* parent)
{
    parent_=parent;
}


void Element::markAsInitialized()
{
    requiresInit_=false;
}

void Element::ensureInitialization() const
{
    if (!isInitialized())
    {
        if (this->hasParent())
        {
            // this will lead to re-entry into ensureInitialization()
            this->parent().ensureInitialization();
        }

        // intialized myself
        if (!isInitialized())
        {
            auto nct=const_cast<Element*>(this);
            nct->markAsInitialized(); // mark first to prevent recursion
            nct->initialize();
        }

        // children will be initialized upon request
    }
}

void Element::resetInitialization()
{
    requiresInit_=true;
}



void Element::initialize()
{
    // for (int i=0; i<nChildren(); ++i)
    // {
    //     if (auto *cp=dynamic_cast<Element*>(&childElementRef(i)))
    //         cp->initialize();
    // }
}


bool Element::hasWorkingDirectory() const
{
    return parent().hasWorkingDirectory();
}

boost::filesystem::path Element::workingDirectory() const
{
    return parent().workingDirectory();
}

void Element::setWorkingDirectory(const boost::filesystem::path& wd) const
{
    parent().setWorkingDirectory(wd);
}


Element::Element(int order)
:   valueChangeSignalBlocked_(false),
    parent_(nullptr),
    order_(order),
    requiresInit_(true)
{}



Element::~Element()
{}


bool Element::isInitialized() const
{
    return !requiresInit_;
}



bool Element::canSetDataFromString() const
{
    return false;
}

void Element::setDataFromString(const std::string&, bool *)
{}

bool Element::isBooleanData() const
{
    return false;
}

bool Element::canSetFromBoolean() const
{
    return false;
}

bool Element::getAsBoolean() const
{
    return false;
}

void Element::setBoolean(bool)
{}

std::string Element::plainTextRepresentation(int indent) const
{
    return "("+type()+")";
}

bool Element::hasParent() const
{
    return parent_.valid();
}



Element& Element::parent()
{
    return parent_;
}

const Element& Element::parent() const
{
    return parent_;
}



std::string Element::path(bool redirectArrayElementsToDefault) const
{
    if (hasParent())
    {
        auto pp=parent().path(redirectArrayElementsToDefault);
        auto n=name(redirectArrayElementsToDefault);
        return pp+(!(pp.empty()||n.empty())?"/":"")+n;
    }
    return std::string();
}


std::string Element::name(bool redirectArrayElementsToDefault) const
{
    if (hasParent())
    {
        return parent().childElementName(this, redirectArrayElementsToDefault);
    }
    else
        return std::string();
}


rapidxml::xml_node<>* Element::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps ) const
{
    using namespace rapidxml;

    if (name.empty())
    {
        return &node;
    }
    else
    {
        xml_node<>* child = doc.allocate_node(
            node_element, doc.allocate_string(
                              this->type().c_str()));

        node.append_node(child);
        appendAttribute(doc, *child, "name", name);

        if (!outProps.skipParameterDescription)
        {
            appendAttribute(doc, *child, "order", order_);
        }

        return child;
    }
}

const rapidxml::xml_node<>* Element::readFromNode(
    const std::string &name,
    const rapidxml::xml_node<> &node )
{

    const rapidxml::xml_node<>* child{nullptr};
    if (!name.empty())
    {
        child = findNode(node, name, type());
    }
    else
    {
        child = &node;
    }

    if (child)
    {
        if (auto oa=getOptionalAttribute(*child, "order"))
        {
            order_=insight::toNumber<double>(*oa);
        }
    }
    return child;
}



void Element::saveToNode(
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& rootNode,
    const OutputProperties& outProps ) const
{
    CurrentExceptionContext ex(
        Loops,
        "writing parameter %s content into XML node",
        type().c_str());

    // store parameters
    appendToNode(std::string(), doc, rootNode, outProps);
}




void Element::saveToStream(
    std::ostream& os,
    const OutputProperties& outProps ) const
{
    CurrentExceptionContext ex(
        Loops,
        "writing element %s content into output stream",
        type().c_str() );

    // prepare XML document
    XMLDocument doc;

    saveToNode(doc, *doc.rootNode, outProps);

    doc.saveToStream(os);
}



void Element::saveToFile(
    const boost::filesystem::path& file,
    const OutputProperties& outProps ) const
{
    CurrentExceptionContext ex(
        insight::VerbosityLevel::BasicBusiness,
        "writing parameter set to file %s", file.string().c_str());

    std::ofstream f(file.c_str());
    saveToStream(f, outProps);
    f << std::endl;
    f << std::flush;
    f.close();
}


void Element::saveToString(
    std::string &s,
    const OutputProperties& outProps ) const
{
    std::ostringstream os(s, std::ios_base::ate);
    saveToStream( os, outProps );
    s = os.str();
}



void Element::readFromRootNode(
    const rapidxml::xml_node<>& rootNode,
    const std::string& startAtSubnode )
{
    CurrentExceptionContext ex("reading parameter %s from XML node", type().c_str());

    auto* crn=&rootNode;
    if (!startAtSubnode.empty())
    {
        std::vector<std::string> path;
        boost::split(path, startAtSubnode, boost::is_any_of("/"));
        for (const auto& p: path)
        {
            std::map<std::string, rapidxml::xml_node<>*> nodes;
            for (auto *e = crn->first_node(); e!=nullptr; e=e->next_sibling())
            {
                nodes[ e->first_attribute("name")->value() ]=e;
            }

            auto e = nodes.find(p);
            if (e==nodes.end())
            {
                std::ostringstream os;
                for(auto& n: nodes) os<<" "<<n.first;
                throw insight::Exception(
                    "Could not find node "+p+" (full path "+startAtSubnode+")!\n"
                    "Available:"+os.str());
            }
            else
            {
                crn=e->second;
            }
        }
    }

    readFromNode(std::string(), *crn);
}



void Element::readFromStream(
    std::istream &is )
{
    CurrentExceptionContext ex(
        "reading element of type %s from input stream",
        type().c_str() );

    XMLDocument doc(is);

    readFromRootNode(*doc.rootNode);
}




void Element::readFromFile(
    const boost::filesystem::path &file,
    const std::string &startAtSubnode )
{
    CurrentExceptionContext ex(
        "reading element of type %s from file %s",
        type().c_str(), file.string().c_str() );

    XMLDocument doc(file);

    readFromRootNode(
        *doc.rootNode,
        startAtSubnode );
}



void Element::readFromString(
    const std::string &contents,
    const std::string &startAtSubnode)
{
    XMLDocument doc(contents.begin(), contents.end());
    readFromRootNode(*doc.rootNode, startAtSubnode);
}



void Element::assignFrom(const Element &rhs)
{
    order_=rhs.order_;

    resetInitialization();

    if (!valueChangeSignalBlocked()) valueChanged();
}



void Element::copyMatching(const Element &rhs)
{
    //just assign, if there are no children
    if (nChildren()==0)
    {
        if (type()==rhs.type())
        {
            assignFrom(rhs);
        }
    }
    else
    {
        throw insight::Exception(
            "internal error: extend function needs to be implemented!");
    }
}


void Element::extend(const Element &op)
{
    // nothing to do, if there are no children
    insight::assertion(
        nChildren()==0,
        "internal error: extend function needs to be implemented!");
}



// void Element::operator=(const Element& rhs)
// {
//     assignFrom(rhs);
// }



std::string Element::childElementName( int i, bool ) const
{
    if (nChildren()!=0)
        throw insight::Exception("internal error: childParameterName() not implemented!");
    return std::string();
}



std::string Element::childElementName(
    const Element *childParam,
    bool redirectArrayElementsToDefault ) const
{
    for (int k=0; k<nChildren(); ++k)
    {
        if (childParam==&childElement(k))
        {
            return childElementName(k, redirectArrayElementsToDefault);
        }
    }

    {
        std::vector<std::string> cands;
        for (int k=0; k<nChildren(); ++k)
            cands.push_back(childElementName(k));

        throw insight::Exception(
            "Parameter %d not found in children list. Candidates are: %s",
            childParam,
            boost::join(cands, ", ").c_str() );
    }
    return std::string();
}


Element &Element::childElementRef(int i)
{
    if (nChildren()!=0)
        throw insight::Exception("internal error: childParameterRef() not implemented!");
    return *this;
}

const Element &Element::childElement(int i) const
{
    if (nChildren()!=0)
        throw insight::Exception("internal error: childParameter() not implemented!");
    return *this;
}


int Element::childElementIndex(const std::string& name) const
{
    for (int k=0; k<nChildren(); ++k)
    {
        if (childElementName(k)==name) return k;
    }
    return -1;
}


int Element::childElementIndex( const Element* childParam ) const
{
    for (int k=0; k<nChildren(); ++k)
    {
        if (&childElement(k)==childParam)
            return k;
    }
    return -1;
}


Element& Element::childElementByNameRef ( const std::string& name )
{
    int i=childElementIndex(name);
    insight::assertion(
        i!=-1,
        "no element with name %s", name.c_str() );
    return childElementRef(i);
}


const Element& Element::childElementByName ( const std::string& name ) const
{
    int i=childElementIndex(name);
    insight::assertion(
        i!=-1,
        "no element with name %s", name.c_str() );
    return childElement(i);
}


std::vector<std::string> Element::childElementNameList() const
{
    std::vector<std::string> res;
    for (int i=0; i<nChildren(); ++i)
    {
        res.push_back(childElementName(i));
    }
    return res;
}


std::vector<std::string> Element::childElementFullPathList() const
{
    auto res = childElementNameList();
    for (auto &r: res)
    {
        r=elementPath::join({path(), r});
    }
    return res;
}


Element::iterator Element::begin()
{
    return iterator(*this, 0);
}

Element::const_iterator Element::begin() const
{
    return cbegin();
}

Element::const_iterator Element::cbegin() const
{
    return const_iterator(*this, 0);
}

Element::iterator Element::end()
{
    return iterator(*this, nChildren());
}

Element::const_iterator Element::end() const
{
    return cend();
}

Element::const_iterator Element::cend() const
{
    return const_iterator(*this, nChildren());
}



Element::UpdateValueSignalBlockage::UpdateValueSignalBlockage(Element &p)
    : blockedElement(p)
{
    blockedElement.setUpdateValueSignalBlockage(true);
}

Element::UpdateValueSignalBlockage::~UpdateValueSignalBlockage()
{
    blockedElement.setUpdateValueSignalBlockage(false);
}


std::unique_ptr<Element::UpdateValueSignalBlockage> Element::blockUpdateValueSignal()
{
    return std::make_unique<UpdateValueSignalBlockage>(*this);
}




void Element::setUpdateValueSignalBlockage(bool block)
{
    valueChangeSignalBlocked_=block;

    for (int i=0; i<nChildren(); ++i)
    {
        childElementRef(i).setUpdateValueSignalBlockage(block);
    }
}

PlaintextRepresentableValue::~PlaintextRepresentableValue()
{}



Element::OutputProperties::OutputProperties()
    : skipParameterDescription(false)
{}


Element::OutputProperties::OutputProperties(const Filter &f)
    : filter(f),
      skipParameterDescription(false)
{}






} // namespace hierarchicalData
} // namespace insight
