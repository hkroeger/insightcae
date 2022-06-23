#ifndef IQVTKPARAMETERSETDISPLAY_H
#define IQVTKPARAMETERSETDISPLAY_H

#include <QTreeView>
#include <QThread>

#include "toolkit_gui_export.h"
#include "iqvtkiscadmodeldisplay.h"

namespace insight {
class CADParameterSetVisualizer;
}

/**
 * @brief The IQVTKParameterSetDisplay class
 * Represents a union of CAD 3D display and modeltree
 * has interface to display and update multiple parameter set visualizers
 */
class TOOLKIT_GUI_EXPORT IQVTKParameterSetDisplay
 : public IQVTKISCADModelDisplay
{
  Q_OBJECT

  friend class VisualizerThread;

  QThread visualizerThread_;

public:
  IQVTKParameterSetDisplay
  (
      QObject* parent,
      IQCADModel3DViewer* viewer,
      QTreeView* modeltree
  );
  virtual ~IQVTKParameterSetDisplay();

//  void connectVisualizer(std::shared_ptr<insight::CADParameterSetVisualizer> viz);
//  void disconnectVisualizer(std::shared_ptr<insight::CADParameterSetVisualizer> viz);

};

#endif // IQVTKPARAMETERSETDISPLAY_H
