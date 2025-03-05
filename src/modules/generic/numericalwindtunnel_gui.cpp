
#include "cadparametersetvisualizer.h"
#include "numericalwindtunnel.h"
#include "cadfeatures/stl.h"
#include "cadfeatures/box.h"
#include "cadfeatures/transform.h"
#include "cadfeatures/importsolidmodel.h"

namespace insight
{


class NumericalWindtunnel_ParameterSet_Visualizer
    : public AnalysisVisualizer<NumericalWindtunnel>
{
public:
    using AnalysisVisualizer<NumericalWindtunnel>::AnalysisVisualizer;

    void recreateVisualizationElements() override
    {
        std::string geom_file_ext = p().geometry.objectfile->fileName().extension().string();
        boost::to_lower(geom_file_ext);

        cad::FeaturePtr org_geom;

        if (geom_file_ext==".stl" || geom_file_ext==".stlb")
        {
            org_geom = cad::STL::create(p().geometry.objectfile->filePath(), sp().cad_to_cfd_);
        }
        else
        {
            org_geom = cad::Transform::create(
                cad::Import::create(p().geometry.objectfile->filePath()),
                sp().cad_to_cfd_
                );
        }

        addFeature("object", org_geom);


        if (p().mesh.longitudinalSymmetry)
        {
            addFeature
                (
                    "domain",
                    cad::Box::create(
                        cad::matconst(vec3( -sp().Lupstream_, 0, 0)),
                        cad::matconst(vec3( sp().Lupstream_+sp().l_+sp().Ldownstream_, 0, 0)),
                        cad::matconst(vec3( 0, 0, sp().Ldown_+sp().Lup_)),
                        cad::matconst(vec3( 0, sp().Laside_+0.5*sp().w_, 0))
                        ),
                    { insight::Wireframe }
                    );
        }
        else
        {
            addFeature
                (
                    "domain",
                    cad::Box::create(
                        cad::matconst(vec3( -sp().Lupstream_, -0.5*sp().w_-sp().Laside_, 0)),
                        cad::matconst(vec3( sp().Lupstream_+sp().l_+sp().Ldownstream_, 0, 0)),
                        cad::matconst(vec3( 0, 0, sp().Ldown_+sp().Lup_)),
                        cad::matconst(vec3( 0, 2.*sp().Laside_+sp().w_, 0))
                        ),
                    { insight::Wireframe }
                    );
        }


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
};



addToStaticFunctionTable2(
    CADParameterSetModelVisualizer, VisualizerFunctions, visualizerForAnalysis,
    NumericalWindtunnel, &newVisualizer<NumericalWindtunnel_ParameterSet_Visualizer>);


insight::CADParameterSetModelVisualizer::IconFunctions::Add<NumericalWindtunnel>
    addNumericalWindtunnelIcon(
        insight::CADParameterSetModelVisualizer::iconForAnalysis_table(),
        []() {
            return QIcon(":/analysis_numericalwindtunnel.svg");
        } );

}
