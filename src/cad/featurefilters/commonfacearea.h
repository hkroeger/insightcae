#ifndef COMMONFACEAREA_H
#define COMMONFACEAREA_H


#include "cadfeature.h"
#include "featureset.h"
#include "base/exception.h"


namespace insight {
namespace cad {

class hasCommonFaceArea
    : public Filter, public EnableCreateFunction<hasCommonFaceArea>
{
protected:
    FeatureSetPtr f_; // to match

public:
    hasCommonFaceArea(FeaturePtr m);
    hasCommonFaceArea(FeatureSetPtr f);

    bool checkMatch(FeatureID i) const override;

    FilterPtr clone() const override;

};



} // namespace cad
} // namespace insight



#endif // COMMONFACEAREA_H
