#ifndef INTERNALPRESSURELOSSCHARACTERISTICS_H
#define INTERNALPRESSURELOSSCHARACTERISTICS_H

#include "internalpressureloss.h"

namespace insight {


extern RangeParameterList rpl_InternalPressureLossCharacteristics;


class InternalPressureLossCharacteristics
    : public OpenFOAMParameterStudy<InternalPressureLoss, rpl_InternalPressureLossCharacteristics>
{
public:
    declareType("Internal Pressure Loss Characteristic Map");

    InternalPressureLossCharacteristics(
        const std::shared_ptr<supplementedInputDataBase>& sp );

    //    virtual void evaluateForceFits(PlotCurveList& crv) const;
    void evaluateCombinedResults(ResultSet& results) override;

    static AnalysisDescription description()
    { return { typeName,
                "Internal pressure loss calculation for multiple volume fluxes" }; }
};


} // namespace insight

#endif // INTERNALPRESSURELOSSCHARACTERISTICS_H
