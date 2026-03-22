#ifndef QANALYSISTHREAD_H
#define QANALYSISTHREAD_H

#include "toolkit_gui_export.h"


#include <QObject>

#include "base/analysisthread.h"
#include "iqbackgroundtask.h"
#include "progressrelay.h"

namespace insight
{

class TOOLKIT_GUI_EXPORT QAnalysisThread
    : public IQBackgroundTaskInterface,
      public AnalysisThread
{
  Q_OBJECT

public:
  QAnalysisThread(
      const std::string& analysisName,
      const ParameterInput& input,
      ProgressDisplayer *pd
#ifndef SWIG
      ,
      std::function<void(void)> preAction=[]()->void {},
      std::function<void(void)> postAction=[]()->void {},
      std::function<void(std::exception_ptr)> exHdlr = [](std::exception_ptr)->void {}
#endif
      );

#ifndef SWIG
  QAnalysisThread(
      std::function<void(void)> action,
      std::function<void(std::exception_ptr)> exHdlr = [](std::exception_ptr)->void {}
      );
#endif

};

}

#endif // QANALYSISTHREAD_H
