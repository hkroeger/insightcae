#ifndef IQVTKCADMODEL3DVIEWERPLANEPOINTBASEDACTION_H
#define IQVTKCADMODEL3DVIEWERPLANEPOINTBASEDACTION_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor/iqvtkselectconstrainedsketchentity.h"

class IQVTKCADModel3DViewer;


class IQVTKCADModel3DViewerPlanePointBasedAction
    : public OptionalInputReceiver<IQVTKSelectConstrainedSketchEntity>
{

public:
    struct PointProperty
    {
        insight::cad::SketchPointPtr p;
        bool isAnExistingPoint;
        insight::cad::FeaturePtr onFeature;
    };

protected:
    boost::signals2::signal<void(PointProperty)> pointSelected;

    /**
     * @brief applyWizards
     * @param pip3d
     * actual coordinate of the mouse
     * @return
     * SketchPoint after application of wizards
     */
    virtual PointProperty
    applyWizards(const arma::mat& pip3d, insight::cad::FeaturePtr onFeature) const;

    PointProperty
    applyWizards(const QPoint& screenPoint, insight::cad::FeaturePtr onFeature) const;


public:
    IQVTKCADModel3DViewerPlanePointBasedAction(
        IQVTKConstrainedSketchEditor &editor,
        bool allowExistingEntities = true );

    bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

};

#endif // IQVTKCADMODEL3DVIEWERPLANEPOINTBASEDACTION_H
