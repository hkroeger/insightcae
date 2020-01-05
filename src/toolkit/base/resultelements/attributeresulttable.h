#ifndef INSIGHT_ATTRIBUTERESULTTABLE_H
#define INSIGHT_ATTRIBUTERESULTTABLE_H

#include "base/resultelement.h"

namespace insight {


class AttributeTableResult
    : public ResultElement
{
public:
    typedef std::vector<std::string> AttributeNames;
    typedef boost::variant<int, double, std::string> AttributeValue;
    typedef std::vector<AttributeValue> AttributeValues;

protected:
    AttributeNames names_;
    AttributeValues values_;

public:
    declareType ( "AttributeTableResult" );

    AttributeTableResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );

    AttributeTableResult
    (
        AttributeNames names,
        AttributeValues values,
        const std::string& shortDesc,
        const std::string& longDesc,
        const std::string& unit
    );

    inline void setTableData ( AttributeNames names, AttributeValues values )
    {
        names_=names;
        values_=values;
    }

    inline const AttributeNames& names() const
    {
        return names_;
    }
    inline const AttributeValues& values() const
    {
        return values_;
    }

    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;
    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

    /**
     * append the contents of this element to the given xml node
     */
    rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const override;

    void readFromNode
        (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node
        ) override;

    ResultElementPtr clone() const override;
};




ResultElementPtr polynomialFitResult
(
  const arma::mat& coeffs,
  const std::string& xvarName,
  const std::string& shortDesc,
  const std::string& longDesc,
  int minorder=0
);



} // namespace insight

#endif // INSIGHT_ATTRIBUTERESULTTABLE_H
