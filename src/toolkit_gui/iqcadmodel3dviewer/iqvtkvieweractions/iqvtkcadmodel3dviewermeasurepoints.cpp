#include "iqvtkcadmodel3dviewermeasurepoints.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "cadfeature.h"
#include "cadpostprocactions.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkselectcadentity.h"


IQVTKCADModel3DViewerMeasurePoints::IQVTKCADModel3DViewerMeasurePoints(IQVTKCADModel3DViewer &viewWidget)
  : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget)
{
    aboutToBeDestroyed.connect(
        [this](){
            viewer().deactivateSubshapeSelectionAll();
        });
}

QString IQVTKCADModel3DViewerMeasurePoints::description() const
{
    return "Measure distance between points";
}

void IQVTKCADModel3DViewerMeasurePoints::start()
{
  viewer().activateSelectionAll(insight::cad::Vertex);
  //viewer().sendStatus("Please select first point!");

  auto sel = make_viewWidgetAction<IQVTKSelectSubshape>(viewer());
  sel->entitySelected.connect(
      [this](IQVTKCADModel3DViewer::SubshapeData p1)
      {
          if (p1.subshapeType_ == insight::cad::Vertex)
          {
              gp_Pnt p =BRep_Tool::Pnt(p1.feat->vertex(p1.id_));
              std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;
              p1_=insight::cad::matconst(insight::vec3(p));

              auto sel2 = make_viewWidgetAction<IQVTKSelectSubshape>(viewer());
              sel2->entitySelected.connect(
                  [this](IQVTKCADModel3DViewer::SubshapeData p2)
                  {
                      if (p2.subshapeType_ == insight::cad::Vertex)
                      {
                          gp_Pnt p =BRep_Tool::Pnt(p2.feat->vertex(p2.id_));
                          std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;
                          p2_=insight::cad::matconst(insight::vec3(p));

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

                          finishAction();
                      }
                  }
                  );

              launchAction(std::move(sel2));
          }
      }
      );

  launchAction(std::move(sel));
}
