#include "internalpressurelosscharacteristics.h"

namespace insight {


RangeParameterList rpl_InternalPressureLossCharacteristics = { "operation/Q" };


defineType(InternalPressureLossCharacteristics);
Analysis::Add<InternalPressureLossCharacteristics> addInternalPressureLossCharacteristics;


InternalPressureLossCharacteristics::InternalPressureLossCharacteristics(
    const std::shared_ptr<supplementedInputDataBase>& sp  )
    : OpenFOAMParameterStudy<InternalPressureLoss,rpl_InternalPressureLossCharacteristics>(
          sp )
{}



void InternalPressureLossCharacteristics::evaluateCombinedResults(ResultSet &results)
{
    hierarchicalData::Ordering o(0.1);
    std::vector<std::string> headers = { "delta_p" };

    std::string key="deltaPTable";
    const TabularResult& tab
        = static_cast<const TabularResult&>(
            results.insert
            (
                       key,
                       this->table(
                           "", "", "operation/Q",
                           headers, nullptr,
                           TableInputType::DoubleInputParameter)
                       ).setOrder(o.next()));

    arma::mat tabdat=tab.toMat();

    addPlot
        (
            results, this->executionPath(), "chartDeltaP",
            "$Q / (m^3 s^{-1})$", "$\\Delta_p / Pa$",
            {
                PlotCurve(tabdat, "deltaP", "w l not")
            },
            "Chart of pressure loss vs. volume flux"
            ) .setOrder(o.next());

}


} // namespace insight
