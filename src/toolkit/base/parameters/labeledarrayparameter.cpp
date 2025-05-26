#include "labeledarrayparameter.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/tools.h"
#include "base/translations.h"
#include "base/parameters/subsetparameter.h"
#include <algorithm>
#include <iterator>

#include "boost/range/adaptor/map.hpp"
#include "boost/range/algorithm/copy.hpp"

namespace insight {


defineType(LabeledArrayParameter);
addToFactoryTable(Parameter, LabeledArrayParameter);





LabeledArrayParameter::LabeledArrayParameter (
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order )
: Parameter(description, isHidden, isExpert, isNecessary, order),
    labelPattern_("entry_%d")
{}




LabeledArrayParameter::LabeledArrayParameter (
    const Parameter& defaultValue,
    int n,
    const std::string& description,
    bool isHidden,
    bool isExpert,
    bool isNecessary,
    int order )
: Parameter(description, isHidden, isExpert, isNecessary, order),
    labelPattern_("entry_%d")
{
    setDefaultValue(defaultValue.clone(false));
    for (int i=0; i<n; i++)
    {
        appendEmpty();
    }
}


void LabeledArrayParameter::synchronizeKeys()
{
    auto& op = parentSet()
        .get<LabeledArrayParameter>(
            keySourceParameterPath_);


    {
        auto myKeys = keys();
        auto validKeys = op.keys();
        std::set<std::string>  tbr;
        std::set_difference(
            myKeys.begin(), myKeys.end(),
            validKeys.begin(), validKeys.end(),
            std::inserter(tbr, tbr.begin()) );

        for (auto& k: tbr)
        {
            eraseValue(k);
        }
    }
    {
        auto myKeys = keys();
        auto validKeys = op.keys();
        std::set<std::string> tba;
        std::set_difference(
            validKeys.begin(), validKeys.end(),
            myKeys.begin(), myKeys.end(),
            std::inserter(tba, tba.begin()) );

        for (auto& k: tba)
        {
            insertWithDefaults(k);
        }
    }
}


void LabeledArrayParameter::initialize()
{
    if (needsInitialization_)
    {
        if (!keySourceParameterPath_.empty())
        {
            synchronizeKeys();

            auto& op = parentSet()
                           .get<LabeledArrayParameter>(
                               keySourceParameterPath_);

            op.newItemAdded.connect(
                [this](const std::string& label, std::observer_ptr<Parameter>)
                {
                    getOrInsertDefaultValue(label);
                });
            op.itemRemoved.connect(
                [this](const std::string& label)
                {
                    if (value_.count(label))
                        eraseValue(label);
                });
            op.itemRelabeled.connect(
                [this](const std::string& label, const std::string& newLabel)
                {
                    if (value_.count(label))
                        changeLabel(label, newLabel);
                });
        }
        needsInitialization_=false;
    }

    Parameter::initialize();
}


bool LabeledArrayParameter::isDifferent(const Parameter& p) const
{
    if (const auto* ap = dynamic_cast<const LabeledArrayParameter*>(&p))
    {
        if (ap->size()!=size())
            return true;

        for (int i=0; i<size(); ++i)
        {
            if ( childParameter(i).isDifferent( ap->childParameter(i) ) )
                return true;
        }

        return false;
    }
    else
        return true;
}


void LabeledArrayParameter::setLabelPattern(const std::string& pat)
{
    labelPattern_=pat;
}


void LabeledArrayParameter::setKeySourceParameterPath(const std::string& pp)
{
    keySourceParameterPath_=pp;
    needsInitialization_=true;
}

void LabeledArrayParameter::unsetKeySourceParameterPath()
{
    setKeySourceParameterPath(std::string());
    needsInitialization_=true;
}

std::set<std::string> LabeledArrayParameter::keys() const
{
    std::set<std::string> r;
    std::transform(
        value_.begin(), value_.end(),
        std::inserter(r, r.begin()),
        [](const value_type::value_type& v){return v.first;} );
    return r;
}




void LabeledArrayParameter::setDefaultValue (
    std::unique_ptr<Parameter>&& defP )
{
    defaultValue_ = std::move(defP);
}




const Parameter& LabeledArrayParameter::defaultValue() const
{
    return *defaultValue_;
}



bool LabeledArrayParameter::keysAreLocked() const
{
    return !keySourceParameterPath_.empty();
}



std::string LabeledArrayParameter::findUniqueNewKey() const
{
    for (int i=size(); i<INT_MAX; ++i)
    {
        auto label =
            str(boost::format(labelPattern_)%i);
        if (value_.count(label)==0)
            return label;
    }
    throw insight::Exception("too many attempts: no suitable key found!");
    return std::string();
}




void LabeledArrayParameter::eraseValue ( const std::string& label )
{
    insight::assertion(
        value_.count(label),
        "entry %s is not present (valid keys are: %s)",
        label.c_str(),
        containerKeyList_to_string(value_, 10).c_str() );

    auto idx=value_.find(label);

    int i = std::distance(value_.begin(), idx);

    beforeChildRemoval(i, i);

    valueChangedConnections_.erase(idx->second.get());
    childValueChangedConnections_.erase(idx->second.get());
    itemRemoved(idx->first);

    auto item=std::move(*idx); // don't delete yet, this would cause crash in IQParameterSetModel.
    value_.erase ( idx ); // Just remove from array

    childRemovalDone(i, i);

    triggerValueChanged();
}




void LabeledArrayParameter::appendValue (
    std::unique_ptr<Parameter>&& np )
{
    insertValue( findUniqueNewKey(), std::move(np) );
}




void LabeledArrayParameter::insertValue (
    const std::string& label,
    std::unique_ptr<Parameter>&& np )
{
    auto i = predictInsertionLocation(value_, label);

    beforeChildInsertion(i, i);

    auto ins=value_.insert({ label, std::move(np) });

    valueChangedConnections_.insert(ins.first->second.get(),
        std::make_shared<boost::signals2::scoped_connection>(
            ins.first->second->valueChanged.connect( childValueChanged )));
    childValueChangedConnections_.insert(ins.first->second.get(),
        std::make_shared<boost::signals2::scoped_connection>(
            ins.first->second->childValueChanged.connect( childValueChanged )));
    newItemAdded(ins.first->first, ins.first->second);
    ins.first->second->setParent(this);

    childInsertionDone(i, i);

    triggerValueChanged();
}



Parameter &LabeledArrayParameter::getOrInsertDefaultValue(const std::string &label)
{
    auto i=value_.find(label);
    if (i!=value_.end())
    {
        return *i->second;
    }
    else
    {
        insertValue(label, defaultValue_->clone(false));
        return *value_.at(label);
    }
}




void LabeledArrayParameter::appendEmpty()
{
    appendValue(defaultValue_->clone(false));
}

void LabeledArrayParameter::insertWithDefaults(const std::string &label)
{
    auto i=value_.find(label);
    if (i==value_.end())
    {
        insertValue(label, defaultValue_->clone(false));
    }
}




void LabeledArrayParameter::changeLabel(
    const std::string &label,
    const std::string &newLabel )
{
    auto idx=value_.find(label);

    insight::assertion(
        idx!=value_.end(),
        "entry %s is not present (valid keys are: %s)",
        label.c_str(),
        containerKeyList_to_string(value_, 10).c_str() );

    std::swap(value_[newLabel], idx->second);
    value_.erase(idx);

    itemRelabeled(label, newLabel);

    triggerValueChanged();
}




Parameter& LabeledArrayParameter::operator[] ( const std::string& label)
{
    return childParameterRef(childParameterIndex(label));
}




const Parameter& LabeledArrayParameter::operator[] ( const std::string& label ) const
{
    return childParameter(childParameterIndex(label));
}




int LabeledArrayParameter::size() const
{
    return value_.size();
}




int LabeledArrayParameter::nChildren() const
{
   return size();
}




std::string LabeledArrayParameter::childParameterName(
    int i, bool ) const
{
    if (i<nChildren())
    {
        auto ii=value_.begin();
        std::advance(ii, i);
        return ii->first;
    }
    else
    {
        throw insight::Exception(
            "index %d out of range (0...%d)", i, nChildren() );
        return std::string();
    }
}




Parameter& LabeledArrayParameter::childParameterRef ( int i )
{
    insight::assertion(i>=0 && i<=nChildren(),
                       "index %d out of range (0...%d)", i, nChildren());
    auto ii=value_.begin();
    std::advance(ii, i);
    return *(ii->second);
}




const Parameter& LabeledArrayParameter::childParameter( int i ) const
{
    insight::assertion(i>=0 && i<=nChildren(),
                       "index %d out of range (0...%d)", i, nChildren());
    auto ii=value_.begin();
    std::advance(ii, i);
    return *(ii->second);
}




int LabeledArrayParameter::childParameterIndex( const std::string& name ) const
{
    auto ii = value_.find(name);
    if (ii==value_.end())
        return -1;
    else
        return std::distance(value_.begin(), ii);
}




void LabeledArrayParameter::clear()
{
    valueChangedConnections_.clear();
    childValueChangedConnections_.clear();
    for (const auto&v: value_)
    {
        itemRemoved(v.first);
    }
    value_.clear();

    triggerValueChanged();
}




std::string LabeledArrayParameter::latexRepresentation() const
{
    std::ostringstream os;
    if (size()>0)
    {
        os<<"\\begin{itemize}\n";

        for(auto i=value_.begin(); i!=value_.end(); ++i)
        {
            os << "\\item " << i->first << " :\\\\\n" << i->second->latexRepresentation();
        }
        os << "\\end{itemize}\n";
    }
    else
    {
        os << "(empty)\n";
    }
    return os.str();
}




std::string LabeledArrayParameter::plainTextRepresentation(int indent) const
{
    std::ostringstream os;
    if (size()>0)
    {
        os << "\n";
        for(auto i=value_.begin(); i!=value_.end(); i++)
        {
            os << std::string(indent+1, ' ')  << i->first << " :\n"
               << std::string(indent+1, ' ')  << i->second->plainTextRepresentation(indent+1);
        }
    }
    else
    {
        os << "(empty)\n";
    }
    return os.str();
}




bool LabeledArrayParameter::isPacked() const
{
    bool is_packed=false;
    for (const auto& p: value_)
    {
        is_packed |= p.second->isPacked();
    }
    return is_packed;
}




void LabeledArrayParameter::pack()
{
    for (auto& p: value_)
    {
        p.second->pack();
    }
}




void LabeledArrayParameter::unpack(const boost::filesystem::path& basePath)
{
    for (auto& p: value_)
    {
        p.second->unpack(basePath);
    }
}




void LabeledArrayParameter::clearPackedData()
{
    for (auto& p: value_)
    {
        p.second->clearPackedData();
    }
}




rapidxml::xml_node<>* LabeledArrayParameter::appendToNode (
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath ) const
{
    insight::CurrentExceptionContext ex(3, "appending labeled array "+name+" to node "+node.name());
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    // defaultValue_->appendToNode("default", doc, *child, inputfilepath);
    appendAttribute(doc, *child, "labelPattern", labelPattern_);
    for (auto& e: value_)
    {
        e.second->appendToNode(
            e.first,
            doc, *child,
            inputfilepath );
    }
    return child;
}




void LabeledArrayParameter::readFromNode (
    const std::string& name,
    const rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath )
{
    using namespace rapidxml;
    auto* child = findNode(node, name, type());

    if (child)
    {
        std::set<std::string> readKeys;
        for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
        {
            std::string name(e->first_attribute("name")->value());
            readKeys.insert(name);
            getOrInsertDefaultValue(name).readFromNode( name, *child, inputfilepath );
        }

        // remove entries not read
        {
            std::vector<std::string> keys;
            boost::copy(value_ | boost::adaptors::map_keys, std::back_inserter(keys));
            for (const auto& k: keys)
            {
                if (readKeys.count(k)==0)
                {
                    itemRemoved(k);
                    value_.erase(k);
                }
            }
        }

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




std::unique_ptr<Parameter> LabeledArrayParameter::clone(bool initialize) const
{
    auto np=std::make_unique<LabeledArrayParameter>(
        *defaultValue_, 0,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );

    np->setLabelPattern(labelPattern_);
    np->setKeySourceParameterPath(keySourceParameterPath_);

    for (int i=0; i<size(); i++)
    {
        np->insertValue(
            childParameterName(i),
            childParameter(i).clone(false) );
    }

    if (initialize) np->initialize();

    return np;
}




void LabeledArrayParameter::copyFrom(const Parameter& p)
{
    operator=(dynamic_cast<const LabeledArrayParameter&>(p));
}




void LabeledArrayParameter::operator=(const LabeledArrayParameter& op)
{
    labelPattern_=op.labelPattern_;
    (*defaultValue_).copyFrom(*op.defaultValue_);

    // remove entries not present in op
    {
        std::vector<std::string> keys;
        boost::copy(value_ | boost::adaptors::map_keys, std::back_inserter(keys));
        for (const auto& k: keys)
        {
            if (op.value_.count(k)==0)
            {
                itemRemoved(k);
                value_.erase(k);
            }
        }
    }

    for (const auto& ov: op.value_)
    {
        auto myv=value_.find(ov.first);
        if (myv!=value_.end())
        {
            myv->second->copyFrom( *ov.second );
        }
        else
        {
            insertValue(ov.first, ov.second->clone(false) );
        }
    }

    Parameter::copyFrom(op);
}




void LabeledArrayParameter::extend ( const Parameter& op )
{
    throw insight::Exception("not implemented");
}




void LabeledArrayParameter::merge ( const Parameter& other )
{
    throw insight::Exception("not implemented");
}




} // namespace insight
