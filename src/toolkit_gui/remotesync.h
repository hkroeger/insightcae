#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include <QThread>
#include "openfoam/remoteexecution.h"


namespace insight {


class RunSyncToRemote : public QThread
{
  Q_OBJECT

  insight::RemoteExecutionConfig& rec_;

  void run() override;

public:
  RunSyncToRemote(insight::RemoteExecutionConfig& rec);

Q_SIGNALS:
  void progressValueChanged(int progress);
  void progressTextChanged(const QString& text);
  void transferFinished();
};




class RunSyncToLocal : public QThread
{
  Q_OBJECT

  insight::RemoteExecutionConfig& rec_;

  void run() override;

public:
  RunSyncToLocal(insight::RemoteExecutionConfig& rec);

Q_SIGNALS:
  void progressValueChanged(int progress);
  void progressTextChanged(const QString& text);
  void transferFinished();
};


}


#endif // REMOTESYNC_H
