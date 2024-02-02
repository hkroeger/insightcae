#ifndef INSIGHT_RAPIDXML_H
#define INSIGHT_RAPIDXML_H

#include <string>
#include "boost/lexical_cast.hpp"

#include "rapidxml/rapidxml.hpp"


namespace insight {

template<class Type>
void appendAttribute(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const std::string& label,
        const Type& value
        )
{
    node.append_attribute
    (
        doc.allocate_attribute(
                    doc.allocate_string(label.c_str()),
                    doc.allocate_string
                    ( boost::lexical_cast<std::string>(value).c_str() )
                    )
    );
}


rapidxml::xml_node<>& appendNode(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& parent,
        const std::string& label );


} // namespace insight

#endif // INSIGHT_RAPIDXML_H
