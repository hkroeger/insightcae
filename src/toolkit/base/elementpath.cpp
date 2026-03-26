#include "elementpath.h"
#include <boost/algorithm/string.hpp>
#include "base/cppextensions.h"

namespace insight {


ElementPath::ElementPath()
{}

ElementPath::ElementPath(const std::string& p)
{
    boost::split(
        static_cast<std::vector<std::string>&>(*this),
        p,
        boost::is_any_of("/"),
        boost::token_compress_on );
}

/**
     * @brief elementPath
     * join elements together
     * @param ps
     */
ElementPath::ElementPath(const std::vector<std::string>& o)
    : std::vector<std::string>(o.begin(), o.end())
{}

ElementPath ElementPath::operator/(const ElementPath& o) const
{
    ElementPath r(*this);
    std::copy(o.begin(), o.end(), std::last_inserter(r));
    return r;
}

bool ElementPath::isBelow(const ElementPath &other) const
{
    if (other.size()<=size())
    {
        for (int i=0; i<other.size(); ++i)
        {
            if (other[i]!=(*this)[i])
                return false;
        }
        return true;
    }
    return false;
}

ElementPath ElementPath::operator/(const std::string& o) const
{
    ElementPath r(*this);
    r.push_back(o);
    return r;
}

ElementPath::operator std::string() const
{
    return boost::join(*this, "/");
}



// std::string
// elementPath::join(const std::string& p1, const std::string& p2)
// {
//     return
//         p1
//         + (
//             (!p1.empty()) && (!p2.empty())
//                 ? "/" : ""
//             ) +
//         p2;
// }




// std::string
// elementPath::join(const std::vector<std::string>& ps)
// {
//     std::string result;
//     for (auto& p: ps)
//     {
//         result=join(result, p);
//     }
//     return result;
// }


} // namespace insight
