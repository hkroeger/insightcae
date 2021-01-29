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
        const std::string& longDesc,
        std::shared_ptr<std::string> base64_content = std::shared_ptr<std::string>()
    );


    void writeLatexHeaderCode ( std::ostream& f ) const override;
    void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const override;

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

} // namespace insight

#endif // INSIGHT_FILERESULT_H
