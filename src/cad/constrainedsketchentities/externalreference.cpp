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

ExternalReference::ExternalReference (
    FeaturePtr extRef, const std::string& layerName )
  : ConstrainedSketchEntity(layerName),
    extRef_(extRef)
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

bool ExternalReference::isInside( SelectionRect r) const
{
    return false;
}


void ExternalReference::operator=(const ConstrainedSketchEntity& other)
{
    ExternalReference::operator=(dynamic_cast<const ExternalReference&>(other));
}

ConstrainedSketchEntityPtr ExternalReference::clone() const
{
    auto cl=ExternalReference::create(extRef_, layerName());

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}

void ExternalReference::operator=(const ExternalReference& other)
{
    Feature::operator=(other);
    ConstrainedSketchEntity::operator=(other);
    extRef_=other.extRef_;
}

std::vector<vtkSmartPointer<vtkProp> > ExternalReference::createActor() const
{
    return extRef_->createVTKActors();
}

} // namespace cad
} // namespace insight
