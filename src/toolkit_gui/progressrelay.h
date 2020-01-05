#ifndef PROGRESSRELAY_H
#define PROGRESSRELAY_H

#include "base/progressdisplayer.h"

#include <QObject>

class ProgressRelay
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
