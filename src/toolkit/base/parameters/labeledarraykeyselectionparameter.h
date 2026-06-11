#ifndef INSIGHT_LABELEDARRAYKEYSELECTIONPARAMETER_H
#define INSIGHT_LABELEDARRAYKEYSELECTIONPARAMETER_H

#include "base/parameters/simpleparameter.h"
#include "base/parameters/selectionparameter.h"

#include <set>
#include <string>
#include <vector>

#include <boost/signals2.hpp>


namespace insight {


/**
 * @brief A selection parameter whose items are dynamically drawn from the keys
 * of a sibling LabeledArrayParameter.
 *
 * The path to the referenced LabeledArrayParameter is stored in XML so that
 * the binding survives serialisation.  When the referenced array's key set
 * changes (items added, removed or relabelled) a valueChanged signal is
 * emitted so that UI widgets can refresh their combo-boxes.
 */
class LabeledArrayKeySelectionParameter
    : public StringParameter,
      public SelectionParameterInterface
{
protected:
    std::string arrayParameterPath_;
    std::set<boost::signals2::scoped_connection> syncConnections_;

    void initializeHierarchy() override;

public:
    declareType ( "labeledArrayKeySelection" );

    /// Restore from XML snapshot (arrayPath attribute + value attribute).
    LabeledArrayKeySelectionParameter(const rapidxml::xml_node<> & node);

    /// Construct without a bound array yet (useful for staged construction).
    LabeledArrayKeySelectionParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    /// Construct with a path to the source LabeledArrayParameter and an
    /// initial selection value.
    LabeledArrayKeySelectionParameter (
        const std::string& arrayParameterPath,
        const std::string& defaultSelection,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    bool isDifferent(const Parameter& p) const override;

    void setArrayParameterPath(const std::string& path);
    const std::string& arrayParameterPath() const;

    // --- SelectionParameterInterface ---
    std::vector<std::string> selectionKeys() const override;
    void setSelection(const key_type& sel) override;
    const key_type& selection() const override;

    // --- XML ---
    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override;

    const rapidxml::xml_node<>* readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node ) override;

protected:
    std::unique_ptr<Element> cloneUninitialized() const override;

public:
    void assignFrom(const Element& p) override;

    int nChildren() const override;
};


} // namespace insight

#endif // INSIGHT_LABELEDARRAYKEYSELECTIONPARAMETER_H
