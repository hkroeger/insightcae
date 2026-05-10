#ifndef INSIGHT_ATTRIBUTERESULTTABLE_H
#define INSIGHT_ATTRIBUTERESULTTABLE_H

#include "base/resultelement.h"

namespace insight {


class AttributeTableResult
    : public ResultElement
{
    static SimpleLatex defaultLabelColumnTitle;
    static SimpleLatex defaultValueColumnTitle;
public:
    typedef std::vector<SimpleLatex> AttributeNames;
    typedef boost::variant<int, double, std::string> AttributeValue;
    typedef std::vector<AttributeValue> AttributeValues;

protected:
    SimpleLatex labelColumnTitle_, valueColumnTitle_;
    AttributeNames names_;
    AttributeValues values_;

public:
    declareType ( "AttributeTableResult" );

    AttributeTableResult (
            const std::string& shortdesc,
            const std::string& longdesc,
            const std::string& unit,
            const SimpleLatex& labelColumnTitle = defaultLabelColumnTitle,
            const SimpleLatex& valueColumnTitle = defaultValueColumnTitle );

    AttributeTableResult(
            AttributeNames names,
            AttributeValues values,
            const std::string& shortDesc,
            const std::string& longDesc,
            const std::string& unit,
            const SimpleLatex& labelColumnTitle = defaultLabelColumnTitle,
            const SimpleLatex& valueColumnTitle = defaultValueColumnTitle );

    inline void setTableData ( AttributeNames names, AttributeValues values )
    {
        names_=names;
        values_=values;
    }

    const SimpleLatex& labelColumnTitle() const;
    const SimpleLatex& valueColumnTitle() const;

    inline const AttributeNames& names() const
    {
        return names_;
    }
    inline const AttributeValues& values() const
    {
        return values_;
    }

    std::string value(int i) const;

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    void exportDataToFile (
        const std::string& name,
        const boost::filesystem::path& outputdirectory ) const override;

    /**
     * append the contents of this element to the given xml node
     */
    rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps
    ) const override;

    const rapidxml::xml_node<>* readFromNode
        (
            const std::string& name,
            const rapidxml::xml_node<>& node
        ) override;

    int nChildren() const override;

    bool isEqual(const Element& op) const override;

protected:
    std::unique_ptr<Element> cloneUninitialized() const override;
};




std::unique_ptr<ResultElement> polynomialFitResult
(
  const arma::mat& coeffs,
  const std::string& xvarName,
  const std::string& shortDesc,
  const std::string& longDesc,
  int minorder=0
);



} // namespace insight

#endif // INSIGHT_ATTRIBUTERESULTTABLE_H
