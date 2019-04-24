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
    virtual size_t calcHash() const;
    virtual void build();

public:
    declareType("MaxSurfaceCurvature");
    MaxSurfaceCurvature();

    static FeaturePtr create(FeatureSetPtr faces);

    operator const TopoDS_Edge& () const;


    virtual void insertrule(parser::ISCADParser& ruleset) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;

};


}
}

#endif // MAXSURFACECURVATURE_H
