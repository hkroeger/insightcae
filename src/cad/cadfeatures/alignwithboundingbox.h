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

    struct Alignment {
        FeaturePtr other_;
        VectorPtr direction_;
        AlignLocation atOther_, atThis_;
    };

private:
    FeaturePtr m1_;

    std::vector<Alignment> alignments_;

    gp_Trsf trsf_;

    AlignWithBoundingBox (
        FeaturePtr m1,
        const std::vector<boost::fusion::vector<
            FeaturePtr, VectorPtr, AlignLocation, AlignLocation> >&
                other_direction_atOther_atThis );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "AlignWithBoundingBox" );
    CREATE_FUNCTION(AlignWithBoundingBox);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    bool isTransformationFeature() const override;
    gp_Trsf transformation() const override;
};


} // namespace cad
} // namespace insight

#endif // ALIGNWITHBOUNDINGBOX_H
