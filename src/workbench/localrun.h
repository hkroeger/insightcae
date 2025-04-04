#ifndef LOCALRUN_H
#define LOCALRUN_H

#include "workbenchaction.h"

#include "base/analysis.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "base/boost_include.h"

#include "qanalysisthread.h"

#include <memory>



class LocalRun
    : public WorkbenchAction
{
  Q_OBJECT

  insight::ResultSetPtr results_;
  insight::QAnalysisThread workerThread_;

public:
  LocalRun(AnalysisForm *af);
  ~LocalRun();


public Q_SLOTS:
  void onCancel() override;

};


#endif // LOCALRUN_H
