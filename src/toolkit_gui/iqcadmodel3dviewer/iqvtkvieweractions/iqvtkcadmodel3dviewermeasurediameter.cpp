#include "iqvtkcadmodel3dviewermeasurediameter.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "cadfeature.h"
#include "cadpostprocactions.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkselectcadentity.h"


IQVTKCADModel3DViewerMeasureDiameter::IQVTKCADModel3DViewerMeasureDiameter(IQVTKCADModel3DViewer &viewWidget)
    : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget)
{
    viewer().activateSelectionAll(insight::cad::Edge);
    //viewer().sendStatus("Please select first point!");

    auto sel = std::make_shared<IQVTKSelectSubshape>(viewer());
    sel->entitySelected.connect(
        [this](IQVTKCADModel3DViewer::SubshapeData edg)
        {
            if (edg.subshapeType_ == insight::cad::Edge)
            {
                insight::cad::CircleEdgeCenterCoords cecc(
                    std::make_shared<insight::cad::FeatureSet>(
                        edg.feat, edg.subshapeType_, edg.id_) );

                arma::mat p, ex;
                double D;
                cecc.compute(p, D, ex);
                std::cout<< "ctr = [" << p[0] <<" "<<p[1]<< " " << p[2] << "], D="<<D<<std::endl;
                viewer().appendToNotepad(
                    QString("Circle([%1, %2, %3], [%4, %5, %6], %7)")
                        .arg(p[0]).arg(p[1]).arg(p[2])
                        .arg(ex[0]).arg(ex[1]).arg(ex[2])
                        .arg(D) );

                finishAction();
            }
        }
        );

    launchChildAction(sel);
}

IQVTKCADModel3DViewerMeasureDiameter::~IQVTKCADModel3DViewerMeasureDiameter()
{
    viewer().deactivateSubshapeSelectionAll();
}
