#ifndef INSIGHT_XMLFILE_H
#define INSIGHT_XMLFILE_H

#include "base/boost_include.h"
#include "rapidxml/rapidxml.hpp"


namespace insight {

class XMLFile
 : public rapidxml::xml_document<>
{
    xml_node<> *rootNode_;
public:
    /**
     * @brief XMLFile
     * creates blank XML document
     */
    XMLFile();

    XMLFile( const boost::filesystem::path& fileName );

    inline xml_node<>* rootNode() { return rootNode_; };
    inline const xml_node<>* rootNode() const { return rootNode_; };

    void saveToFile(const boost::filesystem::path& file) const;
};

} // namespace insight

#endif // INSIGHT_XMLFILE_H
