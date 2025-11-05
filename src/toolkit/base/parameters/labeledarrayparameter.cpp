#include "labeledarrayparameter.h"
#include "base/cppextensions.h"
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
addParameterFactories(LabeledArrayParameter);




void LabeledArrayParameter::initialize()
{
    syncConnections_.clear();

    if (!keySourceParameterPath_.empty())
    {
        auto& op = parentSet()
        .get<LabeledArrayParameter>(
            keySourceParameterPath_);

        // auto ukeys = [this]() {
        //     std::set<std::string> r;
        //     std::transform(
        //         value_.begin(), value_.end(),
        //         std::inserter(r, r.begin()),
        //         [](const value_type::value_type& v){return v.first;} );
        //     return r;
        // };

        // auto blocker = blockUpdateValueSignal();

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
                eraseValueImpl(k);
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
                insertWithDefaultsImpl(k);
            }
        }

        syncConnections_.insert(std::move(
            op.newItemAdded.connect(
                [this](const std::string& label, std::observer_ptr<Parameter>)
                {
                    getOrInsertDefaultValueImpl(label);
                })
            ));
        syncConnections_.insert(std::move(
            op.itemRemoved.connect(
                [this](const std::string& label)
                {
                    if (value_.count(label))
                        eraseValueImpl(label);
                })
            ));

        syncConnections_.insert(std::move(
            op.itemRelabeled.connect(
                [this](const std::string& label, const std::string& newLabel)
                {
                    if (value_.count(label))
                        changeLabelImpl(label, newLabel);
                })
            ));
    }

    Parameter::initialize();
}



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
    setDefaultValue(defaultValue.cloneAs<Parameter>());
    for (int i=0; i<n; i++)
    {
        appendEmpty();
    }
}






bool LabeledArrayParameter::isDifferent(const Parameter& p) const
{
    if (const auto* ap = dynamic_cast<const LabeledArrayParameter*>(&p))
    {
        if (ap->size()!=size())
            return true;

        for (int i=0; i<size(); ++i)
        {
            auto tc=dynamic_cast<const Parameter*>(&childElement(i));
            auto otc=dynamic_cast<const Parameter*>(&ap->childElement(i));
            if ( tc && otc && tc->isDifferent(*otc) )
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
    resetInitialization();
}

void LabeledArrayParameter::unsetKeySourceParameterPath()
{
    setKeySourceParameterPath(std::string());
    syncConnections_.clear();
    resetInitialization();
}

bool LabeledArrayParameter::keysAreLocked() const
{
    return !keySourceParameterPath_.empty();
}

std::set<std::string> LabeledArrayParameter::keys() const
{
    ensureInitialization();

    std::set<std::string> r;
    std::transform(
        value_.begin(), value_.end(),
        std::inserter(r, r.begin()),
        [](const value_type::value_type& v){return v.first;} );
    return r;
}

bool LabeledArrayParameter::hasKey(const std::string &label) const
{
    return value().count(label)>0;
}

LabeledArrayParameter::value_type &LabeledArrayParameter::value()
{
    ensureInitialization();
    return value_;
}


const LabeledArrayParameter::value_type &LabeledArrayParameter::value() const
{
    ensureInitialization();
    return value_;
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






std::string LabeledArrayParameter::findUniqueNewKey() const
{
    for (int i=size(); i<INT_MAX; ++i)
    {
        auto label =
            str(boost::format(labelPattern_)%i);
        if (value().count(label)==0)
            return label;
    }
    throw insight::Exception("too many attempts: no suitable key found!");
    return std::string();
}




void LabeledArrayParameter::eraseValue ( const std::string& label )
{
    insight::assertion(
        !keysAreLocked(), "attempt to erase entry from synchronized array");
    ensureInitialization();
    eraseValueImpl(label);
}

void LabeledArrayParameter::eraseValueImpl ( const std::string& label )
{
    auto& v = value();

    insight::assertion(
        v.count(label),
        "entry %s is not present (valid keys are: %s)",
        label.c_str(),
        containerKeyList_to_string(v, 10).c_str() );


    auto idx=v.find(label);

    int i = std::distance(v.begin(), idx);

    beforeChildRemoval(i, i);

    valueChangedConnections_.erase(idx->second.get());
    childValueChangedConnections_.erase(idx->second.get());
    itemRemoved(idx->first);

    auto item=std::move(*idx); // don't delete yet, this would cause crash in IQParameterSetModel.
    v.erase ( idx ); // Just remove from array

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

    insight::assertion(
        !keysAreLocked(),
        "attempt to insert %s entry to synchronized array", label.c_str());
    ensureInitialization();
    insertValueImpl(label, std::move(np));
}

void LabeledArrayParameter::insertValueImpl(
    const std::string& label,
    std::unique_ptr<Parameter>&& np )
{
    auto &v = value();

    auto i = predictInsertionLocation(v, label);

    beforeChildInsertion(i, i);

    auto ins = v.insert({ label, std::move(np) });

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
    insight::assertion(
        !(keysAreLocked()&&(!hasKey(label))),
        "attempt to insert %s entry to synchronized array", label.c_str());
    ensureInitialization();
    return getOrInsertDefaultValueImpl(label);

}

Parameter &LabeledArrayParameter::getOrInsertDefaultValueImpl(const std::string &label)
{
    auto& v=value();

    auto i=v.find(label);
    if (i!=v.end())
    {
        return *i->second;
    }
    else
    {
        insertValueImpl(label, defaultValue_->cloneAs<Parameter>());
        return *v.at(label);
    }
}




void LabeledArrayParameter::appendEmpty()
{
    appendValue(defaultValue_->cloneAs<Parameter>());
}

void LabeledArrayParameter::insertWithDefaults(const std::string &label)
{
    insight::assertion(
        !keysAreLocked(),
        "attempt to insert %s entry to synchronized array", label.c_str());
    ensureInitialization();
    insertWithDefaultsImpl(label);
}

void LabeledArrayParameter::insertWithDefaultsImpl(const std::string &label)
{
    auto& v=value();

    auto i=v.find(label);
    if (i==v.end())
    {
        insertValueImpl(label, defaultValue_->cloneAs<Parameter>());
    }
}




void LabeledArrayParameter::changeLabel(
    const std::string &label,
    const std::string &newLabel )
{
    insight::assertion(
        !keysAreLocked(),
        "attempt to change label %s of entry %s in synchronized array",
        label.c_str(), newLabel.c_str());
    ensureInitialization();
    changeLabelImpl(label, newLabel);
}

void LabeledArrayParameter::changeLabelImpl(
        const std::string &label,
        const std::string &newLabel )
{
    auto& v=value();

    auto idx=v.find(label);

    insight::assertion(
        idx!=v.end(),
        "entry %s is not present (valid keys are: %s)",
        label.c_str(),
        containerKeyList_to_string(v, 10).c_str() );

    std::swap(v[newLabel], idx->second);
    v.erase(idx);

    itemRelabeled(label, newLabel);

    triggerValueChanged();
}




Parameter& LabeledArrayParameter::operator[] ( const std::string& label)
{
    return dynamic_cast<Parameter&>(
        childElementRef(childElementIndex(label)) );
}




const Parameter& LabeledArrayParameter::operator[] ( const std::string& label ) const
{
    return dynamic_cast<const Parameter&>(
        childElement(childElementIndex(label)) );
}




int LabeledArrayParameter::size() const
{
    return value().size();
}




int LabeledArrayParameter::nChildren() const
{
   return size();
}




std::string LabeledArrayParameter::childElementName(
    int i, bool ) const
{
    if (i<nChildren())
    {
        auto ii=value().begin();
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




hierarchicalData::Element& LabeledArrayParameter::childElementRef ( int i )
{
    insight::assertion(i>=0 && i<=nChildren(),
                       "index %d out of range (0...%d)", i, nChildren());
    auto ii=value().begin();
    std::advance(ii, i);
    return *(ii->second);
}




const hierarchicalData::Element& LabeledArrayParameter::childElement( int i ) const
{
    insight::assertion(i>=0 && i<=nChildren(),
                       "index %d out of range (0...%d)", i, nChildren());
    auto ii=value().begin();
    std::advance(ii, i);
    return *(ii->second);
}




int LabeledArrayParameter::childElementIndex( const std::string& name ) const
{
    auto ii = value().find(name);
    if (ii==value().end())
        return -1;
    else
        return std::distance(value().begin(), ii);
}




void LabeledArrayParameter::clear()
{
    insight::assertion(
        !keysAreLocked(),
        "attempt to clear synchronized array");

    valueChangedConnections_.clear();
    childValueChangedConnections_.clear();
    for (const auto&v: value())
    {
        itemRemoved(v.first);
    }
    value().clear();

    triggerValueChanged();
}




std::string LabeledArrayParameter::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
    std::ostringstream os;
    if (size()>0)
    {
        os<<"\\begin{itemize}\n";

        for(auto i=value().begin(); i!=value().end(); ++i)
        {
            os
                << "\\item "
                << SimpleLatex(i->first).toLaTeX()
                << " :\\\\\n"
                << i->second->latexRepresentation(
                       i->first, documentHierarchyLevel, fsi );
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
        for(auto i=value().begin(); i!=value().end(); i++)
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
    for (const auto& p: value())
    {
        is_packed |= p.second->isPacked();
    }
    return is_packed;
}




void LabeledArrayParameter::pack()
{
    for (auto& p: value())
    {
        p.second->pack();
    }
}




void LabeledArrayParameter::unpack(const boost::filesystem::path& basePath)
{
    for (auto& p: value())
    {
        p.second->unpack(basePath);
    }
}




void LabeledArrayParameter::clearPackedData()
{
    for (auto& p: value())
    {
        p.second->clearPackedData();
    }
}




rapidxml::xml_node<>* LabeledArrayParameter::appendToNode (
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node ) const
{
    insight::CurrentExceptionContext ex(
        insight::VerbosityLevel::Loops,
        "appending labeled array %s to node %s", name.c_str(), node.name());

    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node);
    // defaultValue_->appendToNode("default", doc, *child, inputfilepath);
    appendAttribute(doc, *child, "labelPattern", labelPattern_);
    for (auto& e: value())
    {
        e.second->appendToNode(
            e.first,
            doc, *child );
    }
    return child;
}




const rapidxml::xml_node<>*
LabeledArrayParameter::readFromNode (
    const std::string& name,
    const rapidxml::xml_node<>& node )
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
            getOrInsertDefaultValueImpl(name).readFromNode( name, *child );
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

        resetInitialization();

        triggerValueChanged();
    }
    else
    {
        insight::Warning(
            boost::str(
                boost::format(
                    _("No xml node found with type '%s' and name '%s', default value '%s' is used.")
                    ) % type() % name % plainTextRepresentation(0)
                )
            );
    }
    return child;
}



LabeledArrayParameter::LabeledArrayParameter(const rapidxml::xml_node<> &node)
    : Parameter(node),
    defaultValue_(
        std::dynamic_unique_ptr_cast<Parameter>(
            ParameterSet::create())),
    labelPattern_(getOptionalAttributeOrDefault<std::string>(
          node, "labelPattern", "entry_%d"))
{
    for (auto *e = node.first_node(); e; e = e->next_sibling())
    {
        std::string name(e->first_attribute("name")->value());
        insertValue(
            getMandatoryAttribute(*e, "name"),
            Parameter::createFromNode(*e)
        );
    }

    triggerValueChanged();
}



std::unique_ptr<hierarchicalData::Element> LabeledArrayParameter::clone() const
{
    auto np=std::make_unique<LabeledArrayParameter>(
        *defaultValue_, 0,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );

    np->setLabelPattern(labelPattern_);

    for (auto& v: value_)
    {
        np->insertValueImpl(
            v.first,
            v.second->cloneAs<Parameter>() );
    }

    np->setKeySourceParameterPath(keySourceParameterPath_);

    return np;
}




void LabeledArrayParameter::assignFrom(const Element& oe)
{
    auto &op = dynamic_cast<const LabeledArrayParameter&>(oe);

    labelPattern_=op.labelPattern_;
    (*defaultValue_).assignFrom(*op.defaultValue_);

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
            myv->second->assignFrom( *ov.second );
        }
        else
        {
            insertValueImpl(ov.first, ov.second->cloneAs<Parameter>() );
        }
    }

    Parameter::assignFrom(op);
}



void LabeledArrayParameter::copyMatching(const Element& oe)
{
    auto &op = dynamic_cast<const LabeledArrayParameter&>(oe);

    labelPattern_=op.labelPattern_;
    (*defaultValue_).copyMatching(*op.defaultValue_);

    for (const auto& ov: op.value_)
    {
        auto myv=value_.find(ov.first);
        if (myv!=value_.end())
        {
            myv->second->copyMatching( *ov.second );
        }
    }

    resetInitialization();

    Parameter::assignFrom(op);
}



void LabeledArrayParameter::extend ( const Element& op )
{
    throw insight::Exception("not implemented");
}

bool LabeledArrayParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const LabeledArrayParameter*>(&op))
    {

        if (!defaultValue_->isEqual(*oa->defaultValue_))
            return false;
        if (labelPattern_!=oa->labelPattern_)
            return false;
        if (keySourceParameterPath_!=oa->keySourceParameterPath_)
            return false;
        if (size()!=oa->size())
            return false;

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




} // namespace insight
