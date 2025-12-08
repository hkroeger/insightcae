#ifndef WIREFILLET_H
#define WIREFILLET_H

#include "derivedfeature.h"

namespace insight {
namespace cad {



class WireFillet
    : public DerivedFeature
{
    FeatureSetPtr vertices_;
    ScalarPtr r_;

    WireFillet(const WireFillet&o, TreeCloneMap& tcm);
    WireFillet ( FeatureSetPtr vertices, ScalarPtr r );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "WireFillet" );
#ifndef SWIG
    DEPENDS((vertices_,r_));
#endif
    CREATE_FUNCTION(WireFillet);
    CLONEABLE(WireFillet);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};




}
}


#endif // WIREFILLET_H
