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

    inline const std::string& value() const
    {
        return value_;
    }

    ResultElementPtr clone() const override;
};




} // namespace insight

#endif // INSIGHT_COMMENT_H
