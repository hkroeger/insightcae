#include "rapidxml.h"

namespace insight {


// OffspringNode::OffspringNode(
//     rapidxml::xml_node<> &parentNode,
//     rapidxml::xml_node<> &node )
// : std::reference_wrapper<rapidxml::xml_node<> >(node),
//   parentNode_(parentNode)
// {}

// OffspringNode::~OffspringNode()
// {
//     parentNode_.append_node ( &get() );
// }

std::reference_wrapper<rapidxml::xml_node<> >
appendRootNode(
    rapidxml::xml_document<> &doc,
    const std::string &label)
{
    auto *newnode = doc.allocate_node(
        rapidxml::node_element,
        doc.allocate_string(label.c_str()) );
    doc.append_node ( newnode );
    return {*newnode};
}


std::reference_wrapper<rapidxml::xml_node<> >
appendNode(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& parent,
        const std::string& label )
{
    auto *newnode = doc.allocate_node(
                rapidxml::node_element,
                doc.allocate_string(label.c_str()) );
    parent.append_node ( newnode );
    return {*newnode};
}




} // namespace insight
