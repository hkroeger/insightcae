#ifndef INSIGHT_CAD_SINGLEEDGEFEATURE_H
#define INSIGHT_CAD_SINGLEEDGEFEATURE_H



#include "cadfeature.h"


namespace insight {
namespace cad {



class SingleEdgeFeature
        : public Feature
{
public:
    virtual VectorPtr start() const =0;
    virtual VectorPtr end() const =0;

    bool isSingleEdge() const override;
//    bool isSingleCloseWire() const override;
    bool isSingleOpenWire() const override;
};




} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SINGLEEDGEFEATURE_H
