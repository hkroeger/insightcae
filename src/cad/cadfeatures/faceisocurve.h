#ifndef FACEISOCURVE_H
#define FACEISOCURVE_H

#include "cadfeature.h"



namespace insight {
namespace cad {




class FaceIsoCurve
    : public Feature
{
public:
    enum UV { U, V };

private:
    FeatureSetPtr faces_;
    UV coord_;
    ScalarPtr iso_value_;

    FaceIsoCurve ( FeatureSetPtr faces, UV coord, ScalarPtr iso_value );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "FaceIsoCurve" );

    CREATE_FUNCTION(FaceIsoCurve);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

};




}
}

#endif // FACEISOCURVE_H
