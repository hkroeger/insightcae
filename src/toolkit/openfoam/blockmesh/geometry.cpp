#include "geometry.h"

namespace insight {
namespace bmd {


Geometry::Geometry(
        const std::string& label,
        const boost::filesystem::path& fileName)
    : std::string(label),
      fileName_(fileName)
{}

const boost::filesystem::path& Geometry::fileName() const
{
    return fileName_;
}


} // namespace bmd
} // namespace insight
