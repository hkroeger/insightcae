#ifndef INSIGHT_CAD_CUTUP_H
#define INSIGHT_CAD_CUTUP_H

#include "derivedfeature.h"

namespace insight {
namespace cad {

class CutUp
    : public DerivedFeature
{

public:
    typedef VectorPtr Clip;
    typedef std::vector<Clip> Clips;

protected:
    Clips clips_;
    VectorPtr n_;
    ScalarPtr t_;

    CutUp(const CutUp&o, TreeCloneMap& tcm);
    CutUp ( FeaturePtr model, VectorPtr n, ScalarPtr t, Clips clips );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "CutUp" );
#ifndef SWIG
    DEPENDS_W_BASE(DerivedFeature, (clips_, n_, t_));
#endif
    CREATE_FUNCTION(CutUp);
    CLONEABLE(CutUp);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CUTUP_H
