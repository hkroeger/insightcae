#ifndef CONNECTED_H
#define CONNECTED_H

#include "feature.h"
#include "base/exception.h"
#include "constantquantity.h"

namespace insight
{
namespace cad
{

template<EntityType T>
class connected
: public Filter
{
protected:
    /**
     * @brief f_
     * Seed faces, to which all result faces shall be connected topologically.
     */
    FeatureSet f_;

    std::set<FeatureID> selected_feats_;

public:
    connected(FeaturePtr m)
    : f_(m, T)
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    connected(FeatureSet f)
    : f_(f)
    {}

    void initialize(ConstFeaturePtr m)
    {}

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new connected(f_));
    }

};


//template<> connected<Edge>::connected(FeaturePtr m, scalarQuantityComputerPtr tol);
//template<> bool connected<Edge>::checkMatch(FeatureID feature) const;
template<> connected<Face>::connected(FeaturePtr m);
template<> void connected<Face>::initialize(ConstFeaturePtr m);
template<> bool connected<Face>::checkMatch(FeatureID feature) const;
//template<> connected<Solid>::connected(FeaturePtr m, scalarQuantityComputerPtr tol);
//template<> bool connected<Solid>::checkMatch(FeatureID feature) const;

//typedef connected<Edge> connectedEdge;
typedef connected<Face> connectedFace;
//typedef connected<Solid> connectedSolid;


}
}

#endif // CONNECTED_H
