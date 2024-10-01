#ifndef ALIGNWITHBOUNDINGBOX_H
#define ALIGNWITHBOUNDINGBOX_H

#include "cadparameters.h"
#include "derivedfeature.h"

namespace insight {
namespace cad {

class AlignWithBoundingBox
    : public DerivedFeature
{

    FeaturePtr m1_, other_;
    VectorPtr direction_;

    std::shared_ptr<gp_Trsf> trsf_;

    AlignWithBoundingBox ( FeaturePtr m1, FeaturePtr other, VectorPtr direction );

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
