#include "cadparametersetvisualizer.h"
#include "internalpressureloss.h"

#include "datum.h"

namespace insight
{


class InternalPressureLoss_ParameterSet_Visualizer
    : public AnalysisVisualizer<InternalPressureLoss>
{
public:
    typedef InternalPressureLoss::Parameters Parameters;

public:
    using AnalysisVisualizer<InternalPressureLoss>::AnalysisVisualizer;

    void recreateVisualizationElements() override
    {
        for (auto& w: sp().walls_)
        {
            addFeature("walls/"+w.first, w.second, {insight::Surface, vec3(QColorConstants::Gray)});
        }
        addFeature("inlet", sp().inlet_, {insight::Surface, vec3(QColorConstants::Blue)});
        addFeature("outlet", sp().outlet_, {insight::Surface, vec3(QColorConstants::Green)});

        addDatum(
            "PiM",
            std::make_shared<cad::ExplicitDatumPoint>(
                cad::matconst(p().mesh.PiM*1e3) ), true );
    }
};



addToStaticFunctionTable2(
    CADParameterSetModelVisualizer, VisualizerFunctions, visualizerForAnalysis,
    InternalPressureLoss, &newVisualizer<InternalPressureLoss_ParameterSet_Visualizer>);



}
