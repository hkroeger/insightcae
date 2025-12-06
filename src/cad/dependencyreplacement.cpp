
#include "dependencyreplacement.h"
#include "dependencysource.h"

namespace insight {
namespace cad {

void DependencyReplacement::replaceDependencyIn(DependencySource& ds) const
{
    ds.replaceDependency(*this);
}

} // namespace cad
} // namespace insight
