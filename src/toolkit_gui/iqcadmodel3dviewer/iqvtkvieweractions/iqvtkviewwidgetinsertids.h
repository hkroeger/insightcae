#ifndef IQVTKVIEWWIDGETINSERTIDS_H
#define IQVTKVIEWWIDGETINSERTIDS_H

#include "cadfeature.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkselectcadentity.h"

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
      aboutToBeDestroyed.connect(
          [this](){
              viewer().deactivateSubshapeSelectionAll();
          });
  }

  void start() override
  {
      viewer().activateSelectionAll(shapeType);
      //viewer().sendStatus("Please select "+QString(selectionName)+" and finish with right click!");

      auto sel = make_viewWidgetAction<IQVTKSelectSubshape>(viewer());
      sel->entitySelected.connect(
          [this](IQVTKCADModel3DViewer::SubshapeData sd)
          {
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
              }
          }
          );

      launchAction(std::move(sel));
  }


  bool onMouseClick( Qt::MouseButtons btn, Qt::KeyboardModifiers nFlags, const QPoint point ) override
  {
    if ( selection_
          && (btn&Qt::RightButton) )
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
    }
    
    finishAction();

    return true;
  }
};





class IQVTKViewWidgetInsertPointIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Vertex>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertPointIDs(IQVTKCADModel3DViewer &viewWidget);
  QString description() const override;
};





class IQVTKViewWidgetInsertEdgeIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Edge>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertEdgeIDs(IQVTKCADModel3DViewer &viewWidget);
  QString description() const override;
};





class IQVTKViewWidgetInsertFaceIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Face>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertFaceIDs(IQVTKCADModel3DViewer &viewWidget);
  QString description() const override;
};




class IQVTKViewWidgetInsertSolidIDs :
    public IQVTKViewWidgetInsertIDs<insight::cad::Solid>
{

protected:
  insight::cad::FeatureID getId(const insight::cad::Feature& feat, const TopoDS_Shape& s);

public:
  IQVTKViewWidgetInsertSolidIDs(IQVTKCADModel3DViewer &viewWidget);
  QString description() const override;
};




#endif // IQVIEWWIDGETINSERTIDS_H
