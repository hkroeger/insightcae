#ifndef INSIGHT_FILERESULT_H
#define INSIGHT_FILERESULT_H

#include "base/resultelement.h"
#include "base/filecontainer.h"

namespace insight {

class FileResult
    : public ResultElement,
      public FileContainer

{

public:
    declareType ( "File" );

    FileResult
    (
        const std::string& shortdesc,
        const std::string& longdesc,
        const std::string& unit
    );

    FileResult
    (
        const boost::filesystem::path& location,
        const boost::filesystem::path& value,
        const std::string& shortDesc,
        const std::string& longDesc
    );

    FileResult
    (
        const FileContainer& fc,
        const std::string& shortDesc,
        const std::string& longDesc
    );

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    boost::filesystem::path filePath(boost::filesystem::path baseDirectory = "") const;

    /**
     * append the contents of this element to the given xml node
     */
    rapidxml::xml_node<>* appendToNode(
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const override;

    const rapidxml::xml_node< char >* readFromNode(
        const std::string& name,
        const rapidxml::xml_node<>& node
    ) override;

    int nChildren() const override;

    bool isEqual(const Element& op) const override;

    std::unique_ptr<Element> clone() const override;
};

} // namespace insight

#endif // INSIGHT_FILERESULT_H
