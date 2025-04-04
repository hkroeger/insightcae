#ifndef INSIGHT_CAD_CUTUP_H
#define INSIGHT_CAD_CUTUP_H

#include "cadfeature.h"

namespace insight {
namespace cad {

class CutUp
    : public Feature
{

public:
    typedef VectorPtr Clip;
    typedef std::vector<Clip> Clips;

protected:
    FeaturePtr model_;
    Clips clips_;
    VectorPtr n_;
    ScalarPtr t_;

    CutUp ( FeaturePtr model, VectorPtr n, ScalarPtr t, Clips clips );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "CutUp" );

    CREATE_FUNCTION(CutUp);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CUTUP_H
