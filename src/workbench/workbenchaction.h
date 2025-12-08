#ifndef WORKBENCHACTION_H
#define WORKBENCHACTION_H

#include <QObject>

#include "base/exception.h"
#include "base/resultset.h"
#include "base/progressdisplayer.h"

class AnalysisForm;

namespace insight {
class QAnalysisThread;
}

class WorkbenchAction
    // : public QObject
{
  // Q_OBJECT

protected:
  AnalysisForm* af_;

public:
  WorkbenchAction(AnalysisForm* af);
  virtual ~WorkbenchAction();

  virtual std::unique_ptr<insight::ResultSet> moveResults() =0;

// public Q_SLOTS:
  virtual void onCancel() =0;

// Q_SIGNALS:
//   void logMessage(const QString& logmsg);
//   void statusMessage(const QString& msg);

//   void finished();
//   void failed(::std::exception_ptr e);
//   void cancelled();
};

#endif // WORKBENCHACTION_H
