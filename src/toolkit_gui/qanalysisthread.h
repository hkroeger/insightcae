#ifndef QANALYSISTHREAD_H
#define QANALYSISTHREAD_H

#include "toolkit_gui_export.h"


#include <QObject>

#include "base/analysis.h"
#include "progressrelay.h"

namespace insight
{

class TOOLKIT_GUI_EXPORT QAnalysisThread
    : public QObject,
      public AnalysisThread
{
  Q_OBJECT

public:
  QAnalysisThread(
      AnalysisPtr analysis,
      ProgressDisplayer* pd
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

Q_SIGNALS:
  /**
   * @brief finished
   * this is only emitted, if action is successfully completed without exceptions
   */
  void finished(insight::ResultSetPtr results);

  /**
   * @brief failed
   * the action failed with an exception
   * @param exception
   * the exception, which occurred
   */
  void failed(std::exception_ptr exception);

  /**
   * @brief cancelled
   * the action was interrupted
   */
  void cancelled();
};

}

#endif // QANALYSISTHREAD_H
