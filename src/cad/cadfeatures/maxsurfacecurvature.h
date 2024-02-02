#ifndef MAXSURFACECURVATURE_H
#define MAXSURFACECURVATURE_H


#include "cadfeature.h"

namespace insight {
namespace cad {


class MaxSurfaceCurvature
    : public Feature
{
    FeatureSetPtr faces_;

    MaxSurfaceCurvature(FeatureSetPtr faces);

protected:
    size_t calcHash() const override;
    void build() override;

public:
    declareType("MaxSurfaceCurvature");

    CREATE_FUNCTION(MaxSurfaceCurvature);

    operator const TopoDS_Edge& () const;

    static void insertrule(parser::ISCADParser& ruleset) ;
    static FeatureCmdInfoList ruleDocumentation();

};


}
}

#endif // MAXSURFACECURVATURE_H
