#ifndef REMOTESYNC_H
#define REMOTESYNC_H

#include "toolkit_gui_export.h"


#include <QThread>
#include "base/remoteexecution.h"


namespace insight {


class TOOLKIT_GUI_EXPORT RunSyncToRemote
: public QObject,
  public boost::thread
{
  Q_OBJECT

  insight::RemoteExecutionConfig& rec_;
  bool includeProcDirs_;

//  void run() override;

public:
  RunSyncToRemote(insight::RemoteExecutionConfig& rec, bool includeProcDirs);

  void start();
  void wait();

Q_SIGNALS:
  void progressValueChanged(int progress);
  void progressTextChanged(const QString& text);
  void transferFinished();
};




class TOOLKIT_GUI_EXPORT RunSyncToLocal
: public QObject,
  public boost::thread
{
  Q_OBJECT

  insight::RemoteExecutionConfig& rec_;
  bool includeProcDirs_;

//  void run() override;

public:
  RunSyncToLocal(insight::RemoteExecutionConfig& rec, bool includeProcDirs);

  void start();
  void wait();

Q_SIGNALS:
  void progressValueChanged(int progress);
  void progressTextChanged(const QString& text);
  void transferFinished();
};


}


#endif // REMOTESYNC_H
