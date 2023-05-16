#ifndef INSIGHT_CAD_POLYGON_H
#define INSIGHT_CAD_POLYGON_H

#include "cadfeature.h"

namespace insight {
namespace cad {

class Polygon
: public Feature
{
public:
    std::vector<VectorPtr> corners_;

    Polygon ( const std::vector<VectorPtr>& corners );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "Polygon" );
    CREATE_FUNCTION(Polygon);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    bool isSingleClosedWire() const override;
    bool isSingleOpenWire() const override;
};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_POLYGON_H
