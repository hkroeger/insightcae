
#include "base/boost_include.h"

#include "base/cppextensions.h"

namespace std
{

std::size_t hash<boost::filesystem::path>::operator()
    (const boost::filesystem::path& fn) const
{
    size_t h=boost::filesystem::hash_value(fn);
    // build from file path string and last write time (latter only if file exists)
    if (boost::filesystem::exists(fn))
    {
        std::hash_combine(h, boost::filesystem::last_write_time(fn));
    }
    return h;
}

}
