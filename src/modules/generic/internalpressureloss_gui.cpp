#include "cadparametersetvisualizer.h"
#include "internalpressureloss.h"

#include "datum.h"

namespace insight
{


class InternalPressureLoss_ParameterSet_Visualizer
    : public CADParameterSetVisualizer
{
public:
    typedef InternalPressureLoss::Parameters Parameters;

public:
    void recreateVisualizationElements() override;
};


ParameterSetVisualizerPtr InternalPressureLoss_visualizer()
{
    return ParameterSetVisualizerPtr( new InternalPressureLoss_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(Analysis, InternalPressureLoss, visualizer, InternalPressureLoss_visualizer);


void InternalPressureLoss_ParameterSet_Visualizer::recreateVisualizationElements()
{
    CADParameterSetVisualizer::recreateVisualizationElements();


    auto spp=std::make_shared<InternalPressureLoss::supplementedInputData>(
        std::make_unique<InternalPressureLoss::Parameters>(currentParameters()),
        "", *progress_ );

    Q_EMIT updateSupplementedInputData(std::dynamic_pointer_cast<supplementedInputDataBase>(spp));


    for (auto& w: spp->walls_)
    {
        addFeature("walls/"+w.first, w.second, {insight::Surface, vec3(QColorConstants::Gray)});
    }
    addFeature("inlet", spp->inlet_, {insight::Surface, vec3(QColorConstants::Blue)});
    addFeature("outlet", spp->outlet_, {insight::Surface, vec3(QColorConstants::Green)});

    addDatum(
        "PiM",
        std::make_shared<cad::ExplicitDatumPoint>(
            cad::matconst(spp->p().mesh.PiM*1e3) ), true );
}


}
