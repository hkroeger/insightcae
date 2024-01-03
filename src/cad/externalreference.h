#ifndef INSIGHT_CAD_EXTERNALREFERENCE_H
#define INSIGHT_CAD_EXTERNALREFERENCE_H

#include "constrainedsketchgeometry.h"

namespace insight {
namespace cad {

class ExternalReference
: public ConstrainedSketchEntity
{
    FeaturePtr extRef_;

    ExternalReference ( FeaturePtr extRef );

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

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const ExternalReference& other);
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_EXTERNALREFERENCE_H
