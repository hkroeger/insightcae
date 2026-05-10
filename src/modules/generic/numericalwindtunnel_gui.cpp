

#include "numericalwindtunnel_gui.h"
#include "cadfeatures/stl.h"
#include "cadfeatures/box.h"
#include "cadfeatures/transform.h"
#include "cadfeatures/importsolidmodel.h"
#include "blockmeshvisualization.h"
#include "datum.h"


namespace insight
{



void NumericalWindtunnel_ParameterSet_Visualizer::recreateVisualizationElements()
{

    addDatum(
        "PiM",
        std::make_shared<cad::ExplicitDatumPoint>(
            cad::matconst(sp().PiM_) ) );

    for (auto& g: sp().geometry_)
    {
        addFeature("objects/"+g.first, g.second);
    }

    bmd::blockMeshVisualization(
        *sp().blocking).addToVisualizer(*this, "domain");

    int iref=0;
    for (const Parameters::mesh_type::refinementZones_default_type& rz:
         p().mesh.refinementZones)
    {
        if (const auto* bc =
            boost::get<Parameters::mesh_type::refinementZones_default_type::geometry_box_centered_type>(
                &rz.geometry))
        {
            addFeature
                (
                    str(boost::format("refinement/[%d]")%iref),
                    cad::Box::create(
                        cad::matconst(bc->pc),
                        cad::matconst(bc->L*vec3(1,0,0)),
                        cad::matconst(bc->W*vec3(0,1,0)),
                        cad::matconst(bc->H*vec3(0,0,1)),
                        cad::BoxCentering(true, true, true)
                        ),
                    { insight::Wireframe }
                    );
        }
        else if (const auto* b =
                 boost::get<Parameters::mesh_type::refinementZones_default_type::geometry_box_type>(
                       &rz.geometry))
        {
            arma::mat d = b->pmax - b->pmin;
            addFeature
                (
                    str(boost::format("refinement/[%d]")%iref),
                    cad::Box::create(
                        cad::matconst(b->pmin),
                        cad::matconst(d(0)*vec3(1,0,0)),
                        cad::matconst(d(1)*vec3(0,1,0)),
                        cad::matconst(d(2)*vec3(0,0,1))
                        ),
                    { insight::Wireframe }
                    );
        }

        iref++;
    }
}



addToStaticFunctionTable2(
    CADParameterSetModelVisualizer,
    VisualizerFunctions, visualizerForAnalysis,
    NumericalWindtunnel, &newVisualizer<NumericalWindtunnel_ParameterSet_Visualizer>);


insight::CADParameterSetModelVisualizer::IconFunctions::Add<NumericalWindtunnel>
    addNumericalWindtunnelIcon(
        &insight::CADParameterSetModelVisualizer::iconForAnalysis,
        []() {
            return QIcon(":/analysis_numericalwindtunnel.svg");
        } );

}
