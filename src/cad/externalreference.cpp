#include "externalreference.h"

namespace insight {
namespace cad {

defineType ( ExternalReference );

size_t ExternalReference::calcHash() const
{
    return extRef_->hash();
}

void ExternalReference::build()
{
    extRef_->checkForBuildDuringAccess();
    Feature::operator=(*extRef_);
}

ExternalReference::ExternalReference ( FeaturePtr extRef )
    : extRef_(extRef)
{}

void ExternalReference::scaleSketch(double scaleFactor)
{}

void ExternalReference::generateScriptCommand(
    ConstrainedSketchScriptBuffer& script,
    const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const
{}

std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
ExternalReference::dependencies() const
{
    return {};
}

void ExternalReference::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity>& entity,
    const std::shared_ptr<ConstrainedSketchEntity>& newEntity)
{}

FeaturePtr ExternalReference::referencedFeature() const
{
    return extRef_;
}

void ExternalReference::operator=(const ConstrainedSketchEntity& other)
{
    ExternalReference::operator=(dynamic_cast<const ExternalReference&>(other));
}

void ExternalReference::operator=(const ExternalReference& other)
{
    Feature::operator=(other);
    ConstrainedSketchEntity::operator=(other);
    extRef_=other.extRef_;
}

} // namespace cad
} // namespace insight
