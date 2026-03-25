#ifndef INSIGHT_RAPIDXML_H
#define INSIGHT_RAPIDXML_H

#include <string>

#include <armadillo>
#include "base/boost_include.h"

#include "base/tools.h"
#include "base/exception.h"
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
                    ( toString<Type>(value).c_str() )
                    )
    );
}



template<class T = std::string>
T getMandatoryAttribute(
    const rapidxml::xml_node<> &node,
    const std::string& attributeName )
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
    {
        return toValue<T>(fn->value());
    }
    else
    {
        throw insight::Exception(
            "node does not have mandatory attribute \"%s\"!",
            attributeName.c_str());
    }
}



template<class T = std::string>
std::shared_ptr<T>
getOptionalAttribute(
    const rapidxml::xml_node<> &node,
    const std::string& attributeName )
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
    {
        return std::make_shared<T>(
            toValue<T>(
                fn->value()));
    }
    else
    {
        return std::shared_ptr<T>();
    }
}

template<class T = std::string>
T
getOptionalAttributeOrDefault(
    const rapidxml::xml_node<> &node,
    const std::string& attributeName,
    T defl )
{
    auto spv = getOptionalAttribute<T>(node, attributeName);
    if (spv)
        return *spv;
    else
        return defl;
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




class XMLDocument
    : public rapidxml::xml_document<>
{
    std::string buffer_; // needs to persist during the lifetime of xml_document
    void parseBuffer();

public:
    xml_node<> *rootNode = nullptr;

    struct RootNodeProperties
    {
        std::string name;
        std::vector<std::pair<std::string, std::string> > attributes;
    };

    /**
     * @brief XMLDocument
     * create empty XML document with a single rootNode named "root"
     */
    XMLDocument(const RootNodeProperties& rootNode = { "root", {} });

    /**
     * @brief XMLDocument
     * parse the specified string. Find the top level node named "root", if it exists.
     * @param file
     */
    template<class Iterator>
    XMLDocument(Iterator beg, Iterator end)
        : buffer_(beg, end)
    {
        parseBuffer();
    }

    /**
     * @brief XMLDocument
     * parse the specified stream. Find the top level node named "root", if it exists.
     * @param file
     */
    XMLDocument(std::istream& ons);

    /**
     * @brief XMLDocument
     * parse the specified file. Find the top level node named "root", if it exists.
     * @param file
     */
    XMLDocument(const boost::filesystem::path& file);

    void saveToStream(std::ostream& os) const;
    void saveToFile(const boost::filesystem::path& file) const;
};




const rapidxml::xml_node<> *
findNode(
    const rapidxml::xml_node<>& father,
    const std::string& name,
    const std::string& typeName
    );




std::set<std::string>
listNodes(
    const rapidxml::xml_node<>& father );




template<class T = std::string>
T getAttribute(
    const rapidxml::xml_node<> &node,
    const std::string& attrLabel )
{
    auto attr = node.first_attribute(attrLabel.c_str());

    insight::assertion(
        bool(attr),
        "could not find attribute \"%s\"", attrLabel.c_str());

    return boost::lexical_cast<T>(attr->value());
}


void writeMatToXMLNode(
    const arma::mat& matrix,
    rapidxml::xml_document< char >& doc,
    rapidxml::xml_node< char >& node
    );



} // namespace insight

#endif // INSIGHT_RAPIDXML_H
