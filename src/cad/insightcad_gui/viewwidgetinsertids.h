#ifndef VIEWWIDGETINSERTIDS_H
#define VIEWWIDGETINSERTIDS_H

#include "viewwidgetaction.h"

#include "cadfeature.h"

#include "qmodelstepitem.h"
#include "qoccviewwidget.h"
#include "occtools.h"
#include "occguitools.h"

template<
    TopAbs_ShapeEnum shapeEnum,
    insight::cad::EntityType shapeType,
    const char* selectionName,
    const char* featSelCmdName
    >
class ViewWidgetInsertIDs
        : public ViewWidgetAction<QoccViewWidget>
{
protected:
  std::shared_ptr<insight::cad::FeatureSet> selection_;

  virtual insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s) =0;

public:
  ViewWidgetInsertIDs(QoccViewWidget &viewWidget)
    : ViewWidgetAction<QoccViewWidget>(viewWidget)
  {
    insight::cad::ActivateAll(viewer().getContext(), shapeEnum);
    viewer().sendStatus("Please select "+QString(selectionName)+" and finish with right click!");
  }

  ~ViewWidgetInsertIDs()
  {
    insight::cad::DeactivateAll(viewer().getContext(), shapeEnum);
  }

  void onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override
  {
    auto context = viewer().getContext();

    context->InitSelected();
    if (context->MoreSelected())
    {
      TopoDS_Shape v = context->SelectedShape();
      if (!selection_)
        {
          if (QFeatureItem *parent =
              dynamic_cast<QFeatureItem*>(
                viewer().getOwnerItem(context->SelectedInteractive()) ) )
            {
              // restrict further selection to current shape
              insight::cad::DeactivateAll( context, shapeEnum );
              context->Activate(
                    parent->ais(*context), AIS_Shape::SelectionMode(shapeEnum) );

              selection_.reset(new insight::cad::FeatureSet(parent->solidmodelPtr(), shapeType));

              insight::cad::FeatureID id = getId(parent->solidmodel(), v); //parent->solidmodel().vertexID(v);
              selection_->add(id);
              viewer().sendStatus( QString::fromStdString(
                    boost::str(boost::format(
                                 "Selected %s %d. Select next %s, end with right click."
                    ) % selectionName % id % selectionName ) ) );
            }
        }
        else
        {
          insight::cad::FeatureID id = getId(*selection_->model(), v); //selection_->model()->vertexID(v);
          selection_->add(id);
          viewer().sendStatus( QString::fromStdString(
                boost::str(boost::format(
                             "Selected %s %d. Select next %s, end with right click."
                ) % selectionName % id % selectionName ) ) );
        }
    }
  }

  void onRightButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override
  {
    QString text = QString::fromStdString(
          selection_->model()->featureSymbolName() + "?" + featSelCmdName + "=("
          );
    int j=0;
    for (insight::cad::FeatureID i: selection_->data())
      {
        text+=QString::number( i );
        if (j++ < selection_->size()-1) text+=",";
      }
    text+=")\n";
    viewer().insertNotebookText(text);

    setFinished();
  }
};


extern char vertexFunctionName[];
extern char vertexFeatSelCmdName[];

class ViewWidgetInsertPointIDs :
    public ViewWidgetInsertIDs<TopAbs_VERTEX, insight::cad::Vertex, vertexFunctionName, vertexFeatSelCmdName>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  ViewWidgetInsertPointIDs(QoccViewWidget &viewWidget);
};


extern char edgeFunctionName[];
extern char edgeFeatSelCmdName[];

class ViewWidgetInsertEdgeIDs :
    public ViewWidgetInsertIDs<TopAbs_EDGE, insight::cad::Edge, edgeFunctionName, edgeFeatSelCmdName>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  ViewWidgetInsertEdgeIDs(QoccViewWidget &viewWidget);
};


extern char faceFunctionName[];
extern char faceFeatSelCmdName[];

class ViewWidgetInsertFaceIDs :
    public ViewWidgetInsertIDs<TopAbs_FACE, insight::cad::Face, faceFunctionName, faceFeatSelCmdName>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  ViewWidgetInsertFaceIDs(QoccViewWidget &viewWidget);
};


extern char solidFunctionName[];
extern char solidFeatSelCmdName[];

class ViewWidgetInsertSolidIDs :
    public ViewWidgetInsertIDs<TopAbs_SOLID, insight::cad::Solid, solidFunctionName, solidFeatSelCmdName>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  ViewWidgetInsertSolidIDs(QoccViewWidget &viewWidget);
};

#endif // VIEWWIDGETINSERTIDS_H
