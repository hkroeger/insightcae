#ifndef IQVTKISCADMODELDISPLAY_H
#define IQVTKISCADMODELDISPLAY_H

#include <QTreeView>
#include <QThread>

#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"


/**
 * @brief The IQVTKParameterSetDisplay class
 * Represents a union of CAD 3D display and modeltree
 * has interface to display and update multiple parameter set visualizers
 */
class IQVTKISCADModelDisplay
 : public QObject
{
  Q_OBJECT

  IQCADItemModel* model_;

  IQCADModel3DViewer* viewer_;
  QTreeView* modeltree_;


public:
  IQVTKISCADModelDisplay
  (
      QObject* parent,
      IQCADItemModel* model,
      IQCADModel3DViewer* viewer,
      QTreeView* modeltree
  );
  virtual ~IQVTKISCADModelDisplay();

  inline IQCADItemModel* model() { return model_; }
  inline IQCADModel3DViewer* viewer() { return viewer_; }
  inline QTreeView* modeltree() { return modeltree_; }

};

#endif // IQVTKISCADMODELDISPLAY_H
