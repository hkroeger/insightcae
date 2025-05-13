#include "iqvtksplitline.h"
#include "base/exception.h"
#include "constrainedsketchentities/sketchpoint.h"
#include "iqvtkconstrainedsketcheditor.h"
#include <memory>


IQVTKSplitLine::IQVTKSplitLine(
    IQVTKConstrainedSketchEditor &editor )
: IQVTKCADModel3DViewerPlanePointBasedAction(editor)
{}


void IQVTKSplitLine::start()
{
    pointSelected.connect(
        [this](PointProperty pp)
        {
            auto sp = std::make_shared_aggr<PointProperty>(pp);

            if (auto hitLine = std::dynamic_pointer_cast<insight::cad::Line>(pp.onFeature))
            {
                // if selection was attracted by line, move point onto that
                auto np=sketch().p3Dto2D(hitLine->projectOntoLine(pp.p->value()));
                pp.p->setCoords2D(np(0), np(1));
            }

            auto lup = sketch().findLinesUnderPoint(
                pp.p->value());

            auto lineOrg=std::dynamic_pointer_cast<insight::cad::Line>(
                lup.filterOne(
                    [](insight::cad::ConstrainedSketch::LineUnderPoint lup)
                        { return lup.lupt!=
                            insight::cad::ConstrainedSketch::LineUnderPointType::OnEnd; } )
                );

            if (lineOrg)
            {
                if (!pp.isAnExistingPoint)
                {
                    sketch().insertGeometry(
                        std::dynamic_pointer_cast
                        <insight::cad::ConstrainedSketchEntity>( sp->p ) );
                }


                Q_EMIT splitPointSelected(sp.get(), lineOrg);

                if (auto p2=std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                        lineOrg->end()))
                {
                    lineOrg->replaceDependency(p2, sp->p);
                    // notify change
                    sketch().geometryChanged(
                        sketch().findGeometry(lineOrg)->first );

                    auto line2 = insight::cad::Line::create(sp->p, p2);

                    sketch().insertGeometry(line2);
                    sketch().invalidate();

                    Q_EMIT splitLineAdded(line2, lineOrg);

                    finishAction();
                }
                else
                {
                    throw insight::Exception("end point of line is not a sketch point!");
                }

            }
        }
        );

}


bool IQVTKSplitLine::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::RightButton)
    {
        finishAction();
        return true;
    }

    return IQVTKCADModel3DViewerPlanePointBasedAction
        ::onMouseClick(btn, nFlags, point);
}
