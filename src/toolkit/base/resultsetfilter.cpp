#include "resultsetfilter.h"
#include "base/exception.h"

namespace insight {




bool ResultSetFilter::matches(const std::string &path) const
{
    dbg()<<"check match "<<path<<std::endl;
    return matchesConstant(path) || matchesRegex(path);
}




bool ResultSetFilter::matchesConstant(const std::string &path) const
{
    for (const auto& e: *this)
    {
        if (auto *c = boost::get<std::string>(&e))
        {
            if (*c == path)
                return true;
        }
    }
    return false;
}




bool ResultSetFilter::matchesRegex(const std::string &path) const
{
    for (const auto& e: *this)
    {
        if (auto *r = boost::get<boost::regex>(&e))
        {
            if (boost::regex_match(path, *r))
                return true;
        }
    }
    return false;
}




void ResultSetFilter::readFromNode (
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node )
{
    for (rapidxml::xml_node<> *e = node.first_node();
         e; e = e->next_sibling())
    {
        std::string type(e->first_attribute("type")->value());
        if (type=="string")
        {
            insert( std::string(e->first_attribute("value")->value()) );
        }
        else if (type=="regex")
        {
            insert( boost::regex(std::string(e->first_attribute("expression")->value())) );
        }
    }
}




/**
 * append the contents of this element to the given xml node
 */
void ResultSetFilter::appendToNode
(
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node
) const
{
    for (const auto& e: *this)
    {
        rapidxml::xml_node<>* child =
                doc.allocate_node(
                    rapidxml::node_element,
                    doc.allocate_string("filterEntry"));
        node.append_node(child);

        if (auto *c = boost::get<std::string>(&e))
        {
            child->append_attribute(
                doc.allocate_attribute(
                            "type", doc.allocate_string ( "string" ) ) );
            child->append_attribute(
                doc.allocate_attribute(
                            "value", doc.allocate_string ( c->c_str() ) ) );
        }
        else if (auto *r = boost::get<boost::regex>(&e))
        {
            child->append_attribute(
                doc.allocate_attribute(
                            "type", doc.allocate_string ( "regex" ) ) );
            child->append_attribute(
                doc.allocate_attribute(
                            "expression", doc.allocate_string ( r->str().c_str() ) ) );
        }
    }
}


} // namespace insight
