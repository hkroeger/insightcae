#include "cadfeature.h"
#include "cadparameters.h"
#include "cadparametersetvisualizer.h"
#include "internalpressureloss.h"

#include "filetemplate.h"

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
        for (auto& g: p().geometry)
        {
            /****
         * inlet
         ****/
            if (auto *in =boost::get<Parameters::geometry_default_type::role_inlet_type>(
                    &g.second.role))
            {
                addFeature("inlets/"+g.first, g.second.file->geometry(),
                           {insight::Surface, vec3(QColorConstants::Red), {}, 0.7});
            }
            /*****
         * outlet
         *****/
            else if (auto *out =boost::get<Parameters::geometry_default_type::role_outlet_type>(
                         &g.second.role))
            {
                addFeature("outlets/"+g.first, g.second.file->geometry(),
                           {insight::Surface, vec3(QColorConstants::DarkYellow), {}, 0.7});
            }
            /*****
         * wall
         *****/
            else if (auto *wall =boost::get<Parameters::geometry_default_type::role_wall_type>(
                         &g.second.role))
            {
                addFeature("walls/"+g.first, g.second.file->geometry(),
                           {insight::Surface, vec3(QColorConstants::Gray), {}, 0.9});
            }
            /*****
         * symmetry
         *****/
            else if (auto *symm =boost::get<Parameters::geometry_default_type::role_symmetry_type>(
                         &g.second.role))
            {
                addFeature("symmetryPlanes/"+g.first, g.second.file->geometry(),
                           {insight::Surface, vec3(QColorConstants::Yellow), {}, 0.9});
            }
            /*****
         * porous zone
         *****/
            else if (auto *vol = boost::get<Parameters::geometry_default_type::role_porousVolume_type>(
                         &g.second.role))
            {
                addFeature("porousZones/"+g.first, g.second.file->geometry(),
                           {insight::Surface, vec3(QColorConstants::Blue), {}, 0.5});
            }
            else if (auto *ref = boost::get<Parameters::geometry_default_type::role_refinementOnly_type>(
                    &g.second.role))
            {
                addFeature("refinements/"+g.first, g.second.file->geometry(),
                           {insight::Surface, vec3(QColorConstants::LightGray), {}, 0.3});
            }
        }
        // addFeature("inlet", sp().inlet_, {insight::Surface, vec3(QColorConstants::Blue)});
        // addFeature("outlet", sp().outlet_, {insight::Surface, vec3(QColorConstants::Green)});

        addDatum(
            "PiM",
            std::make_shared<cad::ExplicitDatumPoint>(
                cad::matconst(p().mesh.PiM*1e3) ), true );
    }
};



addToStaticFunctionTable2(
    CADParameterSetModelVisualizer,
    VisualizerFunctions, visualizerForAnalysis,
    InternalPressureLoss, &newVisualizer<InternalPressureLoss_ParameterSet_Visualizer>);


insight::CADParameterSetModelVisualizer::IconFunctions::Add<InternalPressureLoss>
    addInternalPressureLossIcon(
        &insight::CADParameterSetModelVisualizer::iconForAnalysis,
        []() {
            return QIcon(":/analysis_internalpressureloss.svg");
        } );

insight::CADParameterSetModelVisualizer::IconFunctions::Add<InternalPressureLossCharacteristics>
    addInternalPressureLossCharacteristicsIcon(
        &insight::CADParameterSetModelVisualizer::iconForAnalysis,
        []() {
            return QIcon(":/analysis_internalpressureloss_vs_Q.svg");
        } );

insight::CADParameterSetModelVisualizer::IconFunctions::Add<FileTemplate>
    addFileTemplateIcon(
        &insight::CADParameterSetModelVisualizer::iconForAnalysis,
        []() {
            return QIcon(":/analysis_filetemplate.svg");
        } );

}


struct GenericModulesResInit {
    GenericModulesResInit() {
        Q_INIT_RESOURCE(genericmodules);
    }
    ~GenericModulesResInit() {
        Q_CLEANUP_RESOURCE(genericmodules);
    }
} genericModulesResInit;
