#include "rapidxml.h"

namespace insight {

rapidxml::xml_node<>& appendNode(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& parent,
        const std::string& label )
{
    auto *newnode = doc.allocate_node(
                rapidxml::node_element,
                doc.allocate_string(label.c_str()) );
    parent.append_node ( newnode );
    return *newnode;
}

} // namespace insight
