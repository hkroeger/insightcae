#ifndef INSIGHT_CAD_FEATUREANNOTATION_H
#define INSIGHT_CAD_FEATUREANNOTATION_H

#include "cadtypes.h"
#include "cadpostprocaction.h"
#include "cadfeature.h"

#include "vtkPolyData.h"

namespace insight {
namespace cad {




/**
 * @brief FeatureAnnotation
 *
 * A PostprocAction that permanently marks a feature in the 3D viewer
 * using a mechanism analogous to
 * IQVTKCADModel3DViewer::highlightActors / SilhouetteHighlighter.
 *
 * createVTKRepr() produces:
 *   - An outline actor in the annotation color: the boundary edges of
 *     the feature's triangulation or, if the feature consists of edges only,
 *     the edges themselves. It serves the same visual role
 *     as the SilhouetteHighlighter's vtkPolyDataSilhouette actor.
 *   - A vtkCaptionActor2D with a leader line and the label — mirrors the
 *     TextActorHighlighter pattern.
 *
 * Unlike selection-highlight actors (temporary RAII IQVTKViewerState objects),
 * these actors are persistent for the current visualisation model and are
 * managed via the PostprocAction model-entry lifecycle: they are added when
 * the model entry is created and removed by IQISCADModelRebuilder when the
 * visualisation is rebuilt.
 *
 * Both actors are PickableOff() so mouse clicks reach the underlying
 * feature, allowing the regular red selection silhouette
 * (from highlightActors) to still function normally on top.
 */
class FeatureAnnotation
    : public PostprocAction
{
    FeaturePtr feature_;
    arma::mat attachmentPoint_;
    std::string label_;
    arma::mat color_;

public:
    /**
     * @param color
     * RGB color, components in [0,1]
     */
    FeatureAnnotation(
        FeaturePtr feature,
        const arma::mat& attachmentPoint,
        const std::string& label,
        const arma::mat& color = vec3(1., 165./255., 0.) );

    /**
     * @brief outline
     * the highlighted geometry
     */
    virtual vtkSmartPointer<vtkPolyData> outline() const;

    std::vector<vtkSmartPointer<vtkProp> > createVTKRepr() const override;

    void write(std::ostream&) const override;

    size_t calcHash() const override;

    void build() override;
};




}
}

#endif // INSIGHT_CAD_FEATUREANNOTATION_H
