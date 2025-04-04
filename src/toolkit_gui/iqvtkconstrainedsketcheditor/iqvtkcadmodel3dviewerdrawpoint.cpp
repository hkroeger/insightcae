#include "iqvtkcadmodel3dviewerdrawpoint.h"
#include <qnamespace.h>




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


bool IQVTKCADModel3DViewerDrawPoint::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::RightButton)
    {
        finishAction();
        return true;
    }
    else
        return IQVTKCADModel3DViewerPlanePointBasedAction
            ::onMouseClick( btn, nFlags, point );
}
