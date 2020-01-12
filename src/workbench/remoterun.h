#ifndef REMOTERUN_H
#define REMOTERUN_H


#include "workbenchaction.h"
#include "analyzeclient.h"



class RemoteRun
    : public WorkbenchAction
{
  Q_OBJECT

  boost::thread workerThread_, cancelThread_;
  insight::AnalyzeClient ac_;

  void createRemoteDirectory();
  void launchRemoteAnalysisServer();
  void removeRemoteDirectory();

public:
  RemoteRun(AnalysisForm* af, bool resume=false);
  ~RemoteRun();

public Q_SLOTS:
  void onCancel() override;
};

#endif // REMOTERUN_H
