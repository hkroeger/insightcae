#include "labeledarraykeyselectionparameter.h"
#include "base/parameters/labeledarrayparameter.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/translations.h"

#include <boost/algorithm/string/join.hpp>

namespace insight {


defineType ( LabeledArrayKeySelectionParameter );
addParameterFactories ( LabeledArrayKeySelectionParameter );




void LabeledArrayKeySelectionParameter::initializeHierarchy()
{
    syncConnections_.clear();

    const bool wasBound = isBound_;
    isBound_ = false;

    if (!arrayParameterPath_.empty() && hasParent())
    {
        try
        {
            auto& arr = parentSet()
                .get<LabeledArrayParameter>(arrayParameterPath_);

            // Keep the stored value if it is still valid; otherwise clear it.
            if (!value_.empty() && !arr.hasKey(value_))
                value_.clear();

            syncConnections_.insert(std::move(
                arr.newItemAdded.connect(
                    [this](const std::string&, std::observer_ptr<Parameter>)
                    {
                        triggerValueChanged();
                    })
                ));

            syncConnections_.insert(std::move(
                arr.itemRemoved.connect(
                    [this](const std::string& key)
                    {
                        if (value_ == key)
                            value_.clear();
                        triggerValueChanged();
                    })
                ));

            syncConnections_.insert(std::move(
                arr.itemRelabeled.connect(
                    [this](const std::string& oldKey, const std::string& newKey)
                    {
                        if (value_ == oldKey)
                            value_ = newKey;
                        triggerValueChanged();
                    })
                ));

            isBound_ = true;
        }
        catch (const insight::ElementNotFoundException&)
        {
            // Referenced array not reachable from current position in the
            // hierarchy — fall back to free-text mode.
        }
    }

    if (isBound_ != wasBound)
        triggerValueChanged();

    Parameter::initializeHierarchy();
}




LabeledArrayKeySelectionParameter::LabeledArrayKeySelectionParameter (
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: StringParameter ( description, isHidden, isExpert, isNecessary, order )
{}




LabeledArrayKeySelectionParameter::LabeledArrayKeySelectionParameter (
    const std::string& arrayParameterPath,
    const std::string& defaultSelection,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
: StringParameter ( defaultSelection, description, isHidden, isExpert, isNecessary, order ),
  arrayParameterPath_ ( arrayParameterPath )
{}




bool LabeledArrayKeySelectionParameter::isDifferent(const Parameter& p) const
{
    if (const auto *op =
            dynamic_cast<const LabeledArrayKeySelectionParameter*>(&p))
    {
        return StringParameter::isDifferent(p)
               || (arrayParameterPath_ != op->arrayParameterPath_);
    }
    return true;
}




void LabeledArrayKeySelectionParameter::setArrayParameterPath(
    const std::string& path)
{
    arrayParameterPath_ = path;
    initializeHierarchy();
}




const std::string& LabeledArrayKeySelectionParameter::arrayParameterPath() const
{
    return arrayParameterPath_;
}




std::vector<std::string>
LabeledArrayKeySelectionParameter::selectionKeys() const
{
    if (isBound_)
    {
        auto& arr = parentSet()
            .get<LabeledArrayParameter>(arrayParameterPath_);
        auto keySet = arr.keys();
        return std::vector<std::string>(keySet.begin(), keySet.end());
    }
    return {};
}


bool LabeledArrayKeySelectionParameter::isBound() const
{
    return isBound_;
}




void LabeledArrayKeySelectionParameter::setSelection(const key_type& sel)
{
    if (isBound_)
    {
        insight::assertion(
            contains(sel),
            _("LabeledArrayKeySelectionParameter: key \"%s\" not present in"
              " referenced array!\n Available keys: %s"),
            sel.c_str(),
            boost::join(selectionKeys(), " ").c_str() );
    }

    StringParameter::set(sel);
}




const std::string& LabeledArrayKeySelectionParameter::selection() const
{
    return value_;
}




rapidxml::xml_node<>*
LabeledArrayKeySelectionParameter::appendToNode (
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node,
    const OutputProperties& outProps ) const
{
    insight::CurrentExceptionContext ex(
        insight::VerbosityLevel::Loops,
        "appending labeledArrayKeySelection %s to node %s",
        name.c_str(), node.name() );

    auto child = Parameter::appendToNode(name, doc, node, outProps);
    appendAttribute(doc, *child, "value",     value_);
    appendAttribute(doc, *child, "arrayPath", arrayParameterPath_);
    return child;
}




const rapidxml::xml_node<>*
LabeledArrayKeySelectionParameter::readFromNode (
    const std::string& name,
    const rapidxml::xml_node<>& node )
{
    auto* child = Parameter::readFromNode(name, node);
    if (child)
    {
        value_              = getMandatoryAttribute(*child, "value");
        arrayParameterPath_ = getMandatoryAttribute(*child, "arrayPath");
        triggerValueChanged();
    }
    else
    {
        insight::Warning(
            boost::str(
                boost::format(
                    _("No xml node found with type '%s' and name '%s',"
                      " default value '%s' is used.")
                    ) % type() % name % plainTextRepresentation(0)
                )
            );
    }
    return child;
}




LabeledArrayKeySelectionParameter::LabeledArrayKeySelectionParameter(
    const rapidxml::xml_node<> & node)
    : StringParameter(node)
{
    if (auto* a = node.first_attribute("value"))
        value_ = a->value();
    if (auto* a = node.first_attribute("arrayPath"))
        arrayParameterPath_ = a->value();
}




std::unique_ptr<hierarchicalData::Element>
LabeledArrayKeySelectionParameter::cloneUninitialized() const
{
    return std::make_unique<LabeledArrayKeySelectionParameter>(
        arrayParameterPath_,
        value_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
}




void LabeledArrayKeySelectionParameter::assignFrom(const Element& e)
{
    auto& op =
        dynamic_cast<const LabeledArrayKeySelectionParameter&>(e);

    arrayParameterPath_ = op.arrayParameterPath_;

    StringParameter::assignFrom(op);
}




int LabeledArrayKeySelectionParameter::nChildren() const
{
    return 0;
}




} // namespace insight
