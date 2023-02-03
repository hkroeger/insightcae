#ifndef INSIGHT_BMD_GEOMETRY_H
#define INSIGHT_BMD_GEOMETRY_H

#include "base/boost_include.h"


namespace insight {
namespace bmd {


class Geometry
        : public std::string
{
    boost::filesystem::path fileName_;

public:
    Geometry(const std::string& label, const boost::filesystem::path& fileName);

    const boost::filesystem::path& fileName() const;
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_GEOMETRY_H
