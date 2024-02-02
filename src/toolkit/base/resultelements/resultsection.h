#ifndef INSIGHT_RESULTSECTION_H
#define INSIGHT_RESULTSECTION_H


#include "base/resultelementcollection.h"


namespace insight {


class ResultSection
    : public ResultElementCollection,
      public ResultElement
{
    std::string sectionName_, introduction_;

public:
    declareType ( "ResultSection" );

    ResultSection ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    ResultSection ( const std::string& sectionName, const std::string& introduction=std::string() );

    const std::string& secionName() const;
    const std::string& introduction() const;

    void insertLatexHeaderCode ( std::set<std::string>& h ) const override;
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
            rapidxml::xml_node<>& node
        ) override;

    std::shared_ptr<ResultElement> clone() const override;
};


typedef std::shared_ptr<ResultSection> ResultSectionPtr;



} // namespace insight

#endif // INSIGHT_RESULTSECTION_H
