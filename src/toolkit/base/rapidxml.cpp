#include "rapidxml.h"
#include "base/exception.h"
#include "base/tools.h"
#include "base/translations.h"

#include "rapidxml/rapidxml_print.hpp"

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
getMandatoryAttribute(const rapidxml::xml_node<> &node, const std::string& attributeName)
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
        return std::string(fn->value());
    else
        throw insight::Exception("node does not have mandatory attribute \""+attributeName+"\"!");
}


std::shared_ptr<std::string>
getOptionalAttribute(const rapidxml::xml_node<> &node, const std::string& attributeName)
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




void XMLDocument::parseBuffer()
{
    this->parse<0>(const_cast<char*>(buffer_.c_str()));
    rootNode = this->first_node("root");
}

XMLDocument::XMLDocument()
{
    using namespace rapidxml;
    xml_node<>* decl = allocate_node(node_declaration);
    decl->append_attribute(allocate_attribute("version", "1.0"));
    decl->append_attribute(allocate_attribute("encoding", "utf-8"));
    append_node(decl);

    rootNode = allocate_node(node_element, "root");
    append_node(rootNode);
}



XMLDocument::XMLDocument(std::istream& ins)
{
    insight::CurrentExceptionContext ex(
        "parsing XML from stream" );

    readStreamIntoString(ins, buffer_);
    parseBuffer();
}


XMLDocument::XMLDocument(const boost::filesystem::path &file)
{
    insight::CurrentExceptionContext ex(
        "parsing XML from file %s", file.c_str() );

    if (!boost::filesystem::exists(file))
    {
        std::cerr << std::endl
                  << _("Error: input file does not exist")<<": "<<file
                  <<std::endl<<std::endl;
        exit(-1);
    }

    readFileIntoString(file, buffer_);
    parseBuffer();
}

void XMLDocument::saveToStream(std::ostream &os) const
{
    os << (*this);
    os << std::endl;
    os << std::flush;
}


void XMLDocument::saveToFile(const boost::filesystem::path& file) const
{
    std::ofstream f(file.string());
    saveToStream(f);
    f.close();
}



const rapidxml::xml_node<> *
findNode(
    const rapidxml::xml_node<>& father,
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
