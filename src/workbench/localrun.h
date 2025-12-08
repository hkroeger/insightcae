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
    : public insight::QAnalysisThread,
      public WorkbenchAction
{
  Q_OBJECT

public:
  LocalRun(AnalysisForm *af);
  ~LocalRun();

  std::unique_ptr<insight::ResultSet> moveResults() override;

public Q_SLOTS:
  void onCancel() override;

};


#endif // LOCALRUN_H
