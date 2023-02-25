#include "iqvtkcadmodel3dviewermeasurepoints.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "cadfeature.h"
#include "cadpostprocactions.h"

IQVTKCADModel3DViewerMeasurePoints::IQVTKCADModel3DViewerMeasurePoints(IQVTKCADModel3DViewer &viewWidget)
  : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget)
{
  viewer().activateSelectionAll(insight::cad::Vertex);
  //viewer().sendStatus("Please select first point!");
}

IQVTKCADModel3DViewerMeasurePoints::~IQVTKCADModel3DViewerMeasurePoints()
{
    viewer().deactivateSubshapeSelectionAll();
}

void IQVTKCADModel3DViewerMeasurePoints::onLeftButtonUp(Qt::KeyboardModifiers /*nFlags*/, const QPoint point)
{
    auto clickedItem = viewer().findUnderCursorAt(point);

    if ( const auto *ci =
         boost::get<IQVTKCADModel3DViewer::DisplayedSubshapeData::const_iterator>(&clickedItem) )
    {
        auto sd = (*ci)->second;
        if (sd.subshapeType_ == insight::cad::Vertex)
        {
            gp_Pnt p =BRep_Tool::Pnt(sd.feat->vertex(sd.id_));
            std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;

            if (!p1_)
            {
                p1_=insight::cad::matconst(insight::vec3(p));
                //viewer().sendStatus("Please select second point!");
            }
            else if (!p2_)
            {
                p2_=insight::cad::matconst(insight::vec3(p));
                //viewer().sendStatus("Measurement is created...");

                auto* model = dynamic_cast<IQCADItemModel*>(viewer().model());
                insight::assertion(
                            model!=nullptr,
                            "invalid model type" );

                model->addPostprocAction
                        (
                            "distance measurement",
                            insight::cad::PostprocActionPtr
                            (
                                new insight::cad::Distance(p1_, p2_)
                                )
                            );

                setFinished();
            }
        }
    }
}
