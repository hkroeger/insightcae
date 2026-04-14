#ifndef EXTREMAL_H
#define EXTREMAL_H


#include "featureset.h"
#include <map>

namespace insight {
namespace cad {

class extremal
    : public insight::cad::Filter
{
protected:
    int rank_, lrank_;
    std::shared_ptr<scalarQuantityComputer> qtc_;
    std::multimap<double, FeatureID> ranking_;

    virtual double criterion(FeatureID feature) =0;

public:
    extremal(ScalarArg qtc, int rank=0, int lrank=-1);
    virtual void firstPass(FeatureID feature);
    virtual void initialize(ConstFeaturePtr m);
    virtual bool checkMatch(FeatureID feature) const;
};

} // namespace cad
} // namespace insight

#endif // EXTREMAL_H
