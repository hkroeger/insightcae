#include "rapidxml.h"
#include "base/exception.h"
#include "base/tools.h"

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


XMLDocument::XMLDocument(const boost::filesystem::path &file)
{
    insight::CurrentExceptionContext ex(
        "parsing XML from file %s", file.c_str() );

    std::string content;
    readFileIntoString(file, content);

    try {
        parse<0>(&content[0]);
    }
    catch (...)
    {
        throw insight::Exception(
            "Failed to parse XML from file %s",
            file.string().c_str() );
    }
}




} // namespace insight
