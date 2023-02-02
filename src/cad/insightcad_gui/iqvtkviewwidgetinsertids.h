#ifndef IQVTKVIEWWIDGETINSERTIDS_H
#define IQVTKVIEWWIDGETINSERTIDS_H

#include "cadfeature.h"
#include "viewwidgetaction.h"
#include "iqvtkcadmodel3dviewer.h"


struct FeatureSelCmd {
    std::string functionName, cmdName;
};

extern std::map<insight::cad::EntityType, FeatureSelCmd> featureSelCmds;



template<insight::cad::EntityType shapeType>
class IQVTKViewWidgetInsertIDs
        : public ViewWidgetAction<IQVTKCADModel3DViewer>,
          public ToNotepadEmitter
{
protected:
  std::shared_ptr<insight::cad::FeatureSet> selection_;

  virtual insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s) =0;

public:
  IQVTKViewWidgetInsertIDs(IQVTKCADModel3DViewer &viewWidget)
    : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget)
  {
    viewer().activateSelectionAll(shapeType);
    //viewer().sendStatus("Please select "+QString(selectionName)+" and finish with right click!");
  }

  ~IQVTKViewWidgetInsertIDs()
  {
      viewer().deactivateSubshapeSelectionAll();
  }

  void onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override
  {
      int p[] = {point.x(), point.y()};
      IQVTKCADModel3DViewer::PickedItem clickedItem = viewer().findPicked(p);

      if ( const auto *ci =
           boost::get<IQVTKCADModel3DViewer::DisplayedSubshapeData::const_iterator>(&clickedItem) )
      {
          auto sd = (*ci)->second;
          if (sd.subshapeType_ == shapeType)
          {
              if (!selection_)
              {
                  // restrict further selection to current shape
                  viewer().deactivateSubshapeSelectionAll();
                  viewer().activateSelection( sd.feat, shapeType );

                  selection_.reset(new insight::cad::FeatureSet(sd.feat, shapeType));
              }

              selection_->add(sd.id_);

//              viewer().sendStatus( QString::fromStdString(
//                    boost::str(boost::format(
//                                 "Selected %s %d. Select next %s, end with right click."
//                    ) % selectionName % id % selectionName ) ) );
          }
      }
  }

  void onRightButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override
  {
    auto icmd = featureSelCmds.find(shapeType);

    insight::assertion(
                icmd!=featureSelCmds.end(),
                "unhandled selection" );

    QString text = QString::fromStdString(
          selection_->model()->featureSymbolName() + "?" + icmd->second.cmdName + "=("
          );
    int j=0;
    for (insight::cad::FeatureID i: selection_->data())
      {
        text+=QString::number( i );
        if (j++ < selection_->size()-1) text+=",";
      }
    text+=")";
    Q_EMIT appendToNotepad(text);

    setFinished();
  }
};





class IQVTKViewWidgetInsertPointIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Vertex>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertPointIDs(IQVTKCADModel3DViewer &viewWidget);
};





class IQVTKViewWidgetInsertEdgeIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Edge>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertEdgeIDs(IQVTKCADModel3DViewer &viewWidget);
};





class IQVTKViewWidgetInsertFaceIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Face>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertFaceIDs(IQVTKCADModel3DViewer &viewWidget);
};




class IQVTKViewWidgetInsertSolidIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Solid>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertSolidIDs(IQVTKCADModel3DViewer &viewWidget);
};




#endif // IQVIEWWIDGETINSERTIDS_H
