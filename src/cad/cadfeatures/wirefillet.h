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

    WireFillet ( FeatureSetPtr vertices, ScalarPtr r );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "WireFillet" );
    CREATE_FUNCTION(WireFillet);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};




}
}


#endif // WIREFILLET_H
