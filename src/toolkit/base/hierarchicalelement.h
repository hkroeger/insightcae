#ifndef HIERARCHICALELEMENT_H
#define HIERARCHICALELEMENT_H


#include <string>
#include "base/cppextensions.h"

#include "rapidxml/rapidxml.hpp"


namespace insight {
namespace hierarchicalData {

namespace elementPath {

std::string
join(const std::string& p1, const std::string& p2);

std::string
join(const std::vector<std::string>& ps);

}


class Element
    : public boost::noncopyable,
      public std::observable
{
public:

#ifndef SWIG
    boost::signals2::signal<void()> valueChanged, childValueChanged;
    boost::signals2::signal<void(int, int)> beforeChildInsertion, childInsertionDone;
    boost::signals2::signal<void(int, int)> beforeChildRemoval, childRemovalDone;
#endif


    Element();
};

} // namespace hierarchicalData
} // namespace insight

#endif // HIERARCHICALELEMENT_H
