#ifndef INSIGHT_CAD_SINGLEEDGEFEATURE_H
#define INSIGHT_CAD_SINGLEEDGEFEATURE_H



#include "base/factory.h"
#include "cadfeature.h"


namespace insight {
namespace cad {



class SingleEdgeFeature
        : public Feature
{

public:
    virtual VectorPtr start() const;
    virtual VectorPtr end() const;

    void setShape(const TopoDS_Shape& shape) override;

    bool isSingleEdge() const override;
    bool isSingleOpenWire() const override;
};


class ImportedSingleEdgeFeature
    : public SingleEdgeFeature
{
    boost::variant<TopoDS_Edge,FeatureSetPtr> importSource_;

    ImportedSingleEdgeFeature(TopoDS_Edge e);
    ImportedSingleEdgeFeature(FeatureSetPtr creashapes);

    size_t calcHash() const override;

    void build() override;

public:
    CREATE_FUNCTION(ImportedSingleEdgeFeature);

};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SINGLEEDGEFEATURE_H
