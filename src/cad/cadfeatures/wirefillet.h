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

    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType ( "WireFillet" );
    WireFillet ();

    static FeaturePtr create ( FeatureSetPtr vertices, ScalarPtr r );


    virtual void insertrule ( parser::ISCADParser& ruleset ) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;
};




}
}


#endif // WIREFILLET_H
