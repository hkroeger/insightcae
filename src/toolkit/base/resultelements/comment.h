#ifndef INSIGHT_COMMENT_H
#define INSIGHT_COMMENT_H


#include "base/resultelement.h"


namespace insight {




class Comment
    : public ResultElement
{
protected:
    std::string value_;

public:
    declareType ( "Comment" );

    Comment ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Comment ( const std::string& value, const std::string& shortDesc );

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    std::string plainTextRepresentation(int indent) const override;

    void exportDataToFile (
        const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

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

    inline const std::string& value() const
    {
        return value_;
    }

    int nChildren() const override;

    bool isEqual(const Element& op) const override;

    std::unique_ptr<Element> clone() const override;
};




} // namespace insight

#endif // INSIGHT_COMMENT_H
