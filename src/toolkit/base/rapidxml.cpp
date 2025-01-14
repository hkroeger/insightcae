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



std::string
getMandatoryAttribute(rapidxml::xml_node<> &node, const std::string& attributeName)
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
        return std::string(fn->value());
    else
        throw insight::Exception("node does not have mandatory attribute \""+attributeName+"\"!");
}


std::shared_ptr<std::string>
getOptionalAttribute(rapidxml::xml_node<> &node, const std::string& attributeName)
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
        return std::make_shared<std::string>(fn->value());
    else
        return std::shared_ptr<std::string>();
}


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



rapidxml::xml_node<> *findNode(
    rapidxml::xml_node<>& father,
    const std::string& name,
    const std::string& typeName )
{
    if (name.empty())
    {
        return &father;
    }
    else
    {
        for (
            auto *child = father.first_node(typeName.c_str());
            child;
            child = child->next_sibling(typeName.c_str()) )
        {
            if (child->first_attribute("name")->value() == name)
            {
                return child;
            }
        }
    }

    return nullptr;
}



void writeMatToXMLNode(
    const arma::mat& matrix,
    rapidxml::xml_document< char >& doc,
    rapidxml::xml_node< char >& node )
{
    std::ostringstream voss;
    matrix.save(voss, arma::raw_ascii);

    // set stringified table values as node value
    node.value(doc.allocate_string(voss.str().c_str()));
}



} // namespace insight
