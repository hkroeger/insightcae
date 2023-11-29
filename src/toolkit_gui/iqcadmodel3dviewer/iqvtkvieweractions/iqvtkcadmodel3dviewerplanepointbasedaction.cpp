#include "iqvtkcadmodel3dviewerplanepointbasedaction.h"

#include "datum.h"
#include "cadpostprocactions/angle.h"
#include "cadpostprocactions/pointdistance.h"

using namespace insight;
using namespace insight::cad;


IQVTKCADModel3DViewerPlanePointBasedAction::PointProperty
IQVTKCADModel3DViewerPlanePointBasedAction
    ::applyWizards(const arma::mat& pip3d, insight::cad::FeaturePtr onFeature) const
{
    auto p2=viewer().pointInPlane2D(
        sketch().plane()->plane(),
        pip3d );

    return {
        std::make_shared<SketchPoint>(
            sketch().plane(), p2(0), p2(1) ),
        false, onFeature
    };
}


IQVTKCADModel3DViewerPlanePointBasedAction::PointProperty
IQVTKCADModel3DViewerPlanePointBasedAction::applyWizards(
    const QPoint& screenPoint, insight::cad::FeaturePtr onFeature) const
{
    return applyWizards(
        viewer().pointInPlane3D(
        sketch().plane()->plane(), screenPoint), onFeature );
}



IQVTKCADModel3DViewerPlanePointBasedAction
    ::IQVTKCADModel3DViewerPlanePointBasedAction(
        IQVTKConstrainedSketchEditor &editor )
    : IQVTKSelectConstrainedSketchEntity( editor )
{
    setSelectionFilter(
        [](std::weak_ptr<insight::cad::ConstrainedSketchEntity> se)
        {
            auto s=std::dynamic_pointer_cast<insight::cad::ConstrainedSketchEntity>(se.lock());
            return
                bool(s)
                && !bool(std::dynamic_pointer_cast<insight::cad::DistanceConstraint>(s))
                && !bool(std::dynamic_pointer_cast<insight::cad::AngleConstraint>(s));
        }
        );

    toggleHoveringSelectionPreview(true);

    entitySelected.connect(
        [this](std::weak_ptr<insight::cad::ConstrainedSketchEntity> e)
        {
            if (auto psel = std::dynamic_pointer_cast<insight::cad::SketchPoint>(e.lock()))
            {
                // point under cursor hit: don't apply wizard just forward selection
                pointSelected({
                    psel, true, nullptr
                });
            }
            else
            {
                if (hasLastMouseLocation())
                {
                    // point under cursor hit: don't apply wizard just forward selection
                    auto pip3d = viewer().pointInPlane3D(
                        sketch().plane()->plane(),
                        lastMouseLocation()
                        );
                    auto p2=viewer().pointInPlane2D(
                        sketch().plane()->plane(),
                        pip3d );
                    pointSelected({
                        std::make_shared<SketchPoint>(
                            sketch().plane(), p2(0), p2(1) ),
                        false,
                        std::dynamic_pointer_cast<insight::cad::Feature>(e.lock())
                    });
                }
            }
            clearSelection();
        }
        );
}




bool IQVTKCADModel3DViewerPlanePointBasedAction::onLeftButtonDown(
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    auto ret = IQVTKSelectConstrainedSketchEntity::onLeftButtonDown(nFlags, point);
    if (!ret)
    {
        pointSelected(
            applyWizards(
                viewer().pointInPlane3D(
                    sketch().plane()->plane(),
                    point
                    ), nullptr ) );
        return true;
    }
    return ret;
}
