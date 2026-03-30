#ifndef INSIGHT_RESULTSECTION_H
#define INSIGHT_RESULTSECTION_H


#include "base/resultelementcollection.h"


namespace insight {


class ResultSection
    : public ResultElementCollection
{
    std::string sectionName_, introduction_;

public:
    declareType ( "ResultSection" );

    ResultSection ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    ResultSection ( const std::string& sectionName, const std::string& introduction=std::string() );

    const std::string& secionName() const;
    const std::string& introduction() const;

    void insertLatexHeaderCode ( std::set<std::string>& h ) const override;
    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;
    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override;

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


protected:
    std::unique_ptr<hierarchicalData::Element> cloneUninitialized() const override;
};


typedef std::shared_ptr<ResultSection> ResultSectionPtr;



} // namespace insight

#endif // INSIGHT_RESULTSECTION_H
