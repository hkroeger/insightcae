#include "iqvtkcadmodel3dviewerdrawpoint.h"




using namespace insight;
using namespace insight::cad;




IQVTKCADModel3DViewerDrawPoint::IQVTKCADModel3DViewerDrawPoint(
    IQVTKConstrainedSketchEditor &editor )
    : IQVTKCADModel3DViewerPlanePointBasedAction(editor)
{}

void IQVTKCADModel3DViewerDrawPoint::start()
{
    pointSelected.connect(
        [this](IQVTKCADModel3DViewerPlanePointBasedAction::PointProperty pp)
        {
            if (!pp.isAnExistingPoint)
            {
                sketch().insertGeometry(
                    std::dynamic_pointer_cast
                    <ConstrainedSketchEntity>( pp.p ) );
                Q_EMIT pointAdded(pp);
            }
        }
        );
}


bool IQVTKCADModel3DViewerDrawPoint::onRightButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    finishAction();
    return true;
}
