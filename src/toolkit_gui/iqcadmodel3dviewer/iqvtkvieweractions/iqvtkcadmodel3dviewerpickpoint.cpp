#include "iqvtkcadmodel3dviewerpickpoint.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkSmartPointer.h"
#include "vtkWorldPointPicker.h"
#include <qdebug.h>
#include <qnamespace.h>

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkselectcadentity.h"


IQVTKCADModel3DViewerPickPoint::IQVTKCADModel3DViewerPickPoint(
    ViewWidgetActionHost<IQVTKCADModel3DViewer> &parent )
: ViewWidgetAction<IQVTKCADModel3DViewer>(parent, false)
{
    aboutToBeDestroyed.connect(
        [this](){
            viewer().deactivateSubshapeSelectionAll();
        });
}


QString IQVTKCADModel3DViewerPickPoint::description() const
{
    return "Measure point coordinates";
}


void IQVTKCADModel3DViewerPickPoint::start()
{
    viewer().activateSelectionAll(insight::cad::Vertex);
    //viewer().sendStatus("Please select first point!");

    auto sel = make_viewWidgetAction<IQVTKSelectSubshape>(viewer());
    sel->entitySelected.connect(
        [this](IQVTKCADModel3DViewer::SubshapeData p1)
        {
            if (p1.subshapeType_ == insight::cad::Vertex)
            {
                gp_Pnt pt = BRep_Tool::Pnt(p1.feat->vertex(p1.id_));
                arma::mat p = insight::vec3(pt);
                std::cout<<"picked point at "<<p.t();

                viewer().appendToNotepad(
                    QString("[%1, %2, %3]")
                        .arg(p[0]).arg(p[1]).arg(p[2])
                    );
            }
        }
        );

    launchAction(std::move(sel));
}


bool IQVTKCADModel3DViewerPickPoint::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::RightButton)
    {
        finishAction();
        return true;
    }

    return ViewWidgetAction<IQVTKCADModel3DViewer>
        ::onMouseClick(btn, nFlags, point);
}
