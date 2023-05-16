#ifndef INSIGHT_CAD_SINGLEEDGEFEATURE_H
#define INSIGHT_CAD_SINGLEEDGEFEATURE_H



#include "cadfeature.h"


namespace insight {
namespace cad {



class SingleEdgeFeature
        : public Feature
{

protected:
    SingleEdgeFeature();
    SingleEdgeFeature(TopoDS_Edge e);
    SingleEdgeFeature(FeatureSetPtr creashapes);

public:
    CREATE_FUNCTION(SingleEdgeFeature);

    virtual VectorPtr start() const;
    virtual VectorPtr end() const;

    void setShape(const TopoDS_Shape& shape) override;

    bool isSingleEdge() const override;
    bool isSingleOpenWire() const override;
};




} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SINGLEEDGEFEATURE_H
