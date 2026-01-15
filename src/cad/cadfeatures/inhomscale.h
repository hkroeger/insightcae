#ifndef INHOMSCALE_H
#define INHOMSCALE_H

#include "cadparameters.h"
#include "derivedfeature.h"

namespace insight {
namespace cad {

class InhomScale
: public DerivedFeature
{

    VectorPtr scale_;

    InhomScale(const InhomScale&o, TreeCloneMap& tcm);
    InhomScale(ConstFeaturePtr m1, VectorPtr scale);

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "InhomScale" );
#ifndef SWIG
    DEPENDS_W_BASE(DerivedFeature, (scale_));
#endif
    CREATE_FUNCTION(InhomScale);
    CLONEABLE(InhomScale);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INHOMSCALE_H
