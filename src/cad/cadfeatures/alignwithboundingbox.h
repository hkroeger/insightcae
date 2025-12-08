#ifndef ALIGNWITHBOUNDINGBOX_H
#define ALIGNWITHBOUNDINGBOX_H

#include "cadparameters.h"
#include "derivedfeature.h"

namespace insight {
namespace cad {

class AlignWithBoundingBox
    : public DerivedFeature
{
public:
    enum AlignLocation {
        Max=1, Center=2, Min=3
    };

    struct Alignment : public DependencySource {
        FeaturePtr other_;
        VectorPtr direction_;
        AlignLocation atOther_, atThis_;

        void replaceDependency(const DependencyReplacement& n) override;
        void addDependencies(DependencyList& dl) const override;
        std::shared_ptr<DependencySource>
            shallowClone(TreeCloneMap& tcm) const override;
    };

private:
    std::vector<Alignment> alignments_;

    gp_Trsf trsf_;

    AlignWithBoundingBox(const AlignWithBoundingBox&o, TreeCloneMap& tcm);
    AlignWithBoundingBox (
        FeaturePtr m1,
        const std::vector<boost::fusion::vector<
            FeaturePtr, VectorPtr, AlignLocation, AlignLocation> >&
                other_direction_atOther_atThis );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "AlignWithBoundingBox" );
#ifndef SWIG
    DEPENDS_W_BASE(DerivedFeature, (alignments_));
#endif
    CREATE_FUNCTION(AlignWithBoundingBox);
    CLONEABLE(AlignWithBoundingBox);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    bool isTransformationFeature() const override;
    gp_Trsf transformation() const override;
};


} // namespace cad
} // namespace insight

#endif // ALIGNWITHBOUNDINGBOX_H
