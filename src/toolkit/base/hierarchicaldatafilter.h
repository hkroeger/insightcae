#ifndef INSIGHT_HIERARCHICALDATAFILTER_H
#define INSIGHT_HIERARCHICALDATAFILTER_H

#include <set>
#include <string>
#include <boost/variant.hpp>
#include <boost/regex.hpp>

#include "rapidxml/rapidxml.hpp"

namespace insight {

namespace hierarchicalData {

class Element;

class Filter
 : public std::set< boost::variant<std::string,boost::regex> >
{
public:
    bool matches(const Element& e) const;
    bool matches(const std::string& path) const;
    bool matchesConstant(const std::string& path) const;
    bool matchesRegex(const std::string& path) const;

    void readFromNode (
            const rapidxml::xml_node<>& node );

    /**
     * append the contents of this element to the given xml node
     */
    void appendToNode
    (
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;
};


}
} // namespace insight

#endif // INSIGHT_HIERARCHICALDATAFILTER_H
