#ifndef INSIGHT_RAPIDXML_H
#define INSIGHT_RAPIDXML_H

#include <string>
#include "boost/lexical_cast.hpp"

#include "rapidxml/rapidxml.hpp"


namespace insight {


// class OffspringNode
//     : public std::reference_wrapper<rapidxml::xml_node<> >
// {
//     rapidxml::xml_node<>& parentNode_;
// public:
//     OffspringNode(
//         rapidxml::xml_node<>& parentNode,
//         rapidxml::xml_node<>& node );
//     ~OffspringNode();
// };

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

std::reference_wrapper<rapidxml::xml_node<> >
appendRootNode(
    rapidxml::xml_document<>& doc,
    const std::string& label );

std::reference_wrapper<rapidxml::xml_node<> >
appendNode(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& parent,
        const std::string& label );


} // namespace insight

#endif // INSIGHT_RAPIDXML_H
