#ifndef PROGRESSRELAY_H
#define PROGRESSRELAY_H

#include "toolkit_gui_export.h"


#include "base/progressdisplayer.h"

#include <QObject>

class TOOLKIT_GUI_EXPORT ProgressRelay
 : public QObject,
   public insight::ProgressDisplayer
{
  Q_OBJECT

public:
  ProgressRelay();

  void update(const insight::ProgressState &pi) override;

Q_SIGNALS:
  void progressUpdate(const insight::ProgressState& pi);
};

#endif // PROGRESSRELAY_H
