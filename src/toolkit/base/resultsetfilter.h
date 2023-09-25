#ifndef INSIGHT_RESULTSETFILTER_H
#define INSIGHT_RESULTSETFILTER_H

#include <set>
#include <string>
#include "base/boost_include.h"

#include "rapidxml/rapidxml.hpp"

namespace insight {

class ResultSetFilter
 : public std::set< boost::variant<std::string,boost::regex> >
{
public:
    bool matches(const std::string& path) const;
    bool matchesConstant(const std::string& path) const;
    bool matchesRegex(const std::string& path) const;

    void readFromNode (
            rapidxml::xml_node<>& node );

    /**
     * append the contents of this element to the given xml node
     */
    void appendToNode
    (
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;
};

} // namespace insight

#endif // INSIGHT_RESULTSETFILTER_H
