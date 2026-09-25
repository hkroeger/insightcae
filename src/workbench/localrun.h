#ifndef LOCALRUN_H
#define LOCALRUN_H

#include "workbenchaction.h"

#include "base/analysis.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"
#include "base/boost_include.h"
#include "base/supplementedinputdata.h"

#include "qanalysisthread.h"

#include <memory>



class LocalRun
    : public insight::QAnalysisThread,
      public WorkbenchAction
{
  Q_OBJECT

public:
  /**
   * @param sid
   * supplemented input data, computed from the current parameters
   * (e.g. by the visualizer). If null, it is computed in the analysis thread.
   */
  LocalRun(AnalysisForm *af, insight::supplementedInputDataBasePtr sid);
  ~LocalRun();

  std::unique_ptr<insight::ResultSet> moveResults() override;

public Q_SLOTS:
  void onCancel() override;

};


#endif // LOCALRUN_H
