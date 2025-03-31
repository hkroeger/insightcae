#ifndef INSIGHT_CAD_EXTERNALREFERENCE_H
#define INSIGHT_CAD_EXTERNALREFERENCE_H

#include "cadfeature.h"
#include "constrainedsketchentity.h"

namespace insight {
namespace cad {

class ExternalReference
: public Feature,
  public ConstrainedSketchEntity
{
    FeaturePtr extRef_;

    ExternalReference ( FeaturePtr extRef, const std::string& layerName = std::string() );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "ExternalReference" );

    CREATE_FUNCTION(ExternalReference);

    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;
    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    FeaturePtr referencedFeature() const;

    bool isInside( SelectionRect r) const override;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const ExternalReference& other);

    ConstrainedSketchEntityPtr clone() const override;

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_EXTERNALREFERENCE_H
