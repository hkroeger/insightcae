#ifndef LOCALRUN_H
#define LOCALRUN_H

#include "workbenchaction.h"

#include "base/analysis.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "base/boost_include.h"

#include <memory>



class LocalRun
    : public WorkbenchAction
{
  Q_OBJECT

  boost::thread workerThread_;
  std::shared_ptr<insight::Analysis> analysis_;
  insight::ResultSetPtr results_;

public:
  LocalRun(AnalysisForm *af);
  ~LocalRun();

public Q_SLOTS:
  void onCancel() override;

};


#endif // LOCALRUN_H
