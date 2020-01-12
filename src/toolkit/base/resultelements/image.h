#ifndef INSIGHT_IMAGE_H
#define INSIGHT_IMAGE_H

#include "base/resultelement.h"

namespace insight {


class Image
    : public ResultElement
{
protected:
    boost::filesystem::path imagePath_;

public:
    declareType ( "Image" );

    Image ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    Image ( const boost::filesystem::path& location, const boost::filesystem::path& value, const std::string& shortDesc, const std::string& longDesc );

    inline const boost::filesystem::path& imagePath() const
    {
        return imagePath_;
    }
    inline void setPath ( const boost::filesystem::path& value )
    {
        imagePath_=value;
    }

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

//    virtual void readFromNode
//    (
//        const std::string& name,
//        rapidxml::xml_document<>& doc,
//        rapidxml::xml_node<>& node
//    );

    ResultElementPtr clone() const override;
};



} // namespace insight

#endif // INSIGHT_IMAGE_H
