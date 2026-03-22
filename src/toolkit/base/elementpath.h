#ifndef ELEMENTPATH_H
#define ELEMENTPATH_H

#include <vector>
#include <string>

namespace insight {

class ElementPath
    : public std::vector<std::string>
{
public:
    ElementPath();
    ElementPath(const std::string& p);
    ElementPath(const std::vector<std::string>& ps);

    ElementPath operator/(const ElementPath& o) const;
    ElementPath operator/(const std::string& o) const;

    operator std::string() const;
};


} // namespace insight

#endif // ELEMENTPATH_H
