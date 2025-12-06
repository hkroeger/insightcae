#include "treeclonemap.h"
#include "dependencysource.h"

namespace insight {
namespace cad {


std::shared_ptr<DependencySource>
TreeCloneMap::doShallowClone(const DependencySource* oldPtr)
{
    return oldPtr->shallowClone(*this);
}

bool TreeCloneMap::skipCloning(const DependencySource *cur) const
{
    if (triggerDeps_.size())
    {
        return !cur->indirectlyDependsOn(triggerDeps_);
    }
    else
        return false;
}

void TreeCloneMap::constrainCloning(
    const std::set<const DependencySource *> &triggerDeps )
{
    triggerDeps_=triggerDeps;
}

} // namespace cad
} // namespace insight
