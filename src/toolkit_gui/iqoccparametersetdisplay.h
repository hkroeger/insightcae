#ifndef IQOCCPARAMETERSETDISPLAY_H
#define IQOCCPARAMETERSETDISPLAY_H


#include <QThread>

#include "toolkit_gui_export.h"
#include "iqvtkiscadmodeldisplay.h"

class QoccViewWidget;
class QModelTree;

namespace insight {
class CADParameterSetModelVisualizer;
}

/**
 * @brief The ParameterSetDisplay class
 * Represents a union of CAD 3D display and modeltree
 * has interface to display and update multiple parameter set visualizers
 */
class TOOLKIT_GUI_EXPORT IQOCCParameterSetDisplay
 : public QObject
{
  Q_OBJECT

  friend class VisualizerThread;

  QoccViewWidget* viewer_;
  QModelTree* modeltree_;

  QThread visualizerThread_;

public:
  IQOCCParameterSetDisplay
  (
      QObject* parent,
      QoccViewWidget* viewer,
      QModelTree* modeltree
  );

  inline QoccViewWidget* viewer() { return viewer_; }
  inline QModelTree* modeltree() { return modeltree_; }

  void connectVisualizer(std::shared_ptr<insight::CADParameterSetModelVisualizer> viz);
  void disconnectVisualizer(std::shared_ptr<insight::CADParameterSetModelVisualizer> viz);

};

#endif // IQOCCPARAMETERSETDISPLAY_H
