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

    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType ( "CutUp" );
    CutUp ();

    static FeaturePtr create ( FeaturePtr model, VectorPtr n, ScalarPtr t, Clips clips );


    virtual void insertrule ( parser::ISCADParser& ruleset ) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CUTUP_H
