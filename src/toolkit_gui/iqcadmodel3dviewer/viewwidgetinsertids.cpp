#include "viewwidgetinsertids.h"


//ViewWidgetInsertIDs::ViewWidgetInsertIDs(QoccViewWidget *viewWidget)
//  : viewWidget_(viewWidget)
//{
//  insight::cad::ActivateAll(viewWidget_->getContext(), shapeToSelect().first);
//  viewWidget_->sendStatus("Please select "+selectionName()+" and finish with right click!");
//}

//ViewWidgetInsertIDs::~ViewWidgetInsertIDs()
//{
//  insight::cad::DeactivateAll(viewWidget_->getContext(), shapeToSelect().first);
//}

//void ViewWidgetInsertIDs::onLeftButtonUp(Qt::KeyboardModifiers /*nFlags*/, const QPoint /*point*/,
//        bool lastClickWasDoubleClick)
//{
//  auto context = viewWidget_->getContext();

//  context->InitSelected();
//  if (context->MoreSelected())
//  {
//    TopoDS_Shape v = context->SelectedShape();
//    if (!selection_)
//      {
//        if (QFeatureItem *parent =
//            dynamic_cast<QFeatureItem*>(
//              viewWidget_->getOwnerItem(context->SelectedInteractive()) ) )
//          {
//            // restrict further selection to current shape
//            insight::cad::DeactivateAll( context, shapeToSelect().first );
//            context->Activate(
//                  parent->ais(*context), AIS_Shape::SelectionMode(shapeToSelect().first) );

//            selection_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), shapeToSelect().second));

//            insight::cad::FeatureID id = getId(parent->solidmodel(), v); //parent->solidmodel().vertexID(v);
//            selection_->add(id);
//            viewWidget_->sendStatus( QString::fromStdString(
//                  boost::str(boost::format(
//                               "Selected %s %d. Select next %s, end with right click."
//                  ) % selectionName() % id % selectionName() ) ) );
//          }
//      }
//      else
//      {
//        insight::cad::FeatureID id = getId(*selection_->model(), v); //selection_->model()->vertexID(v);
//        selection_->add(id);
//        viewWidget_->sendStatus( QString::fromStdString(
//              boost::str(boost::format(
//                           "Selected %s %d. Select next %s, end with right click."
//              ) % selectionName() % id % selectionName() ) ) );
//      }
//  }
//}




//void ViewWidgetInsertIDs::onRightButtonDown(Qt::KeyboardModifiers /*nFlags*/, const QPoint /*point*/)
//{
//  QString text = QString::fromStdString(
//        selection_->model()->featureSymbolName() + "?" + functionName() + "+=("
//        );
//  int j=0;
//  for (insight::cad::FeatureID i: selection_->data())
//    {
//      text+=QString::number( i );
//      if (j++ < selection_->size()-1) text+=",";
//    }
//  text+=")\n";
//  viewWidget_->insertNotebookText(text);

//  setFinished();
//}



char vertexFunctionName[] = "vertex";
char vertexFeatSelCmdName[] = "vid";

insight::cad::FeatureID ViewWidgetInsertPointIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.vertexID(s);
}

ViewWidgetInsertPointIDs::ViewWidgetInsertPointIDs(QoccViewWidget &viewWidget)
  : ViewWidgetInsertIDs<TopAbs_VERTEX, insight::cad::Vertex, vertexFunctionName, vertexFeatSelCmdName>(viewWidget)
{}



char edgeFunctionName[] = "edge";
char edgeFeatSelCmdName[] = "eid";

insight::cad::FeatureID ViewWidgetInsertEdgeIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.edgeID(s);
}

ViewWidgetInsertEdgeIDs::ViewWidgetInsertEdgeIDs(QoccViewWidget &viewWidget)
  : ViewWidgetInsertIDs<TopAbs_EDGE, insight::cad::Edge, edgeFunctionName, edgeFeatSelCmdName>(viewWidget)
{}



char faceFunctionName[] = "face";
char faceFeatSelCmdName[] = "fid";

insight::cad::FeatureID ViewWidgetInsertFaceIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.faceID(s);
}

ViewWidgetInsertFaceIDs::ViewWidgetInsertFaceIDs(QoccViewWidget &viewWidget)
  : ViewWidgetInsertIDs<TopAbs_FACE, insight::cad::Face, faceFunctionName, faceFeatSelCmdName>(viewWidget)
{}




char solidFunctionName[] = "solid";
char solidFeatSelCmdName[] = "sid";

insight::cad::FeatureID ViewWidgetInsertSolidIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.solidID(s);
}

ViewWidgetInsertSolidIDs::ViewWidgetInsertSolidIDs(QoccViewWidget &viewWidget)
  : ViewWidgetInsertIDs<TopAbs_SOLID, insight::cad::Solid, solidFunctionName, solidFeatSelCmdName>(viewWidget)
{}
