#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include "toolkit_gui_export.h"


#include <QThread>
#include "base/remoteexecution.h"


namespace insight {


class TOOLKIT_GUI_EXPORT RunSyncToRemote : public QThread
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




class TOOLKIT_GUI_EXPORT RunSyncToLocal : public QThread
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
